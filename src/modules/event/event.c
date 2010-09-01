/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <glib.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"

#include "api.h"
#include "event.h"

MODULE_NAME("event");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The event module implements an observer pattern that's freely attachable to any object");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static void freeEventListenerQueue(void *queue_p);
static void freeEventSubject(void *subject_p);

static GThread *mainthread;

/**
 * A GHashTable that maps objects to other GHashTables which themselves map string identifiers to EventListenerEntry objects with event listeners
 */
static GHashTable *subjects;

MODULE_INIT
{
	mainthread = g_thread_self();
	subjects = g_hash_table_new_full(&g_direct_hash, &g_direct_equal, NULL, &freeEventSubject);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(subjects);
}

/**
 * Attaches an event listener to a subject
 *
 * @param subject		the subject to attach the event listener to
 * @param event			the event to attach the event listener to
 * @param listener		the listener to attach
 * @param custom		custom data passed to the listener when the event is triggered
 */
API void attachEventListener(void *subject, const char *event, EventListener *listener, void *custom)
{
	if(g_thread_self() != mainthread) {
		return;
	}

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) == NULL) { // Create events if it doesn't exist yet
		events = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeEventListenerQueue);
		g_hash_table_insert(subjects, subject, events);
	}

	GQueue *queue;

	if((queue = g_hash_table_lookup(subject, event)) == NULL) { // Create queue if it doesn't exist yet
		queue = g_queue_new();
		g_hash_table_insert(queue, strdup(event), queue);
	}

	EventListenerEntry *entry = ALLOCATE_OBJECT(EventListenerEntry);
	entry->listener = listener;
	entry->custom = custom;

	g_queue_push_tail(queue, entry);
}

/**
 * Detach an event listener from a subject
 *
 * @param subject		the subject to detach the event listener from
 * @param event			the event to detach the event listener from
 * @param listener		the listener to detach
 * @param custom		custom data passed to the listener when the event was triggered
 */
API void detachEventListener(void *subject, const char *event, EventListener *listener, void *custom)
{
	if(g_thread_self() != mainthread) {
		return;
	}

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) != NULL) {
		GQueue *queue;

		if((queue = g_hash_table_lookup(subject, event)) != NULL) {
			for(GList *iter = queue->head; iter != NULL; iter = iter->next) {
				EventListenerEntry *entry = iter->data;

				if(entry->listener == listener && entry->custom == custom) { // matches
					// Remove listener
					g_queue_remove(queue, entry);
					free(entry);

					// Check if this was the last listener
					if(g_queue_get_length(queue) == 0) {
						// Remove event
						g_hash_table_remove(events, event);

						// Check if this was the last event
						if(g_hash_table_size(events) == 0) {
							// Remove subject
							g_hash_table_remove(subjects, subject);
						}
					}

					return;
				}
			}
		}
	}
}

/**
 * Triggers an event and notifies all its listeners
 *
 * @param subject		the subject for which the event should be triggered
 * @param event			the event that should be triggered
 * @param ...			the data to pass to the listeners
 * @result				the number of listeners notified, -1 on error
 */
API int triggerEvent(void *subject, const char *event, ...)
{
	if(g_thread_self() != mainthread) {
		return -1;
	}

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) == NULL) {
		return -1;
	}

	GQueue *queue;

	if((queue = g_hash_table_lookup(subject, event)) == NULL) {
		return -1;
	}

	int counter = 0;

	for(GList *iter = queue->head; iter != NULL; iter = iter->next, counter++) {
		va_list args;
		// Get args and listener
		va_start(args, event);
		EventListenerEntry *entry = iter->data;

		// Notify listener
		entry->listener(subject, event, entry->custom, args);
	}

	return counter;
}

/**
 * A GDestroyNotify function to free an event listener queue
 *
 * @param queue_p			a pointer to the queue to frees
 */
static void freeEventListenerQueue(void *queue_p)
{
	GQueue *queue = queue_p;

	for(GList *iter = queue->head; iter != NULL; iter = iter->next) {
		free(iter->data);
	}

	g_queue_free(queue);
}

/**
 * A GDestroyNotify wrapper around g_hash_table_destroy to destroy an event subject
 *
 * @param subject_p		a pointer to the subject to free
 */
static void freeEventSubject(void *subject)
{
	g_hash_table_destroy((GHashTable *) subject);
}
