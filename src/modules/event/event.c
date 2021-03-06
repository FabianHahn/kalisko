/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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

#define API
#include "event.h"

MODULE_NAME("event");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The event module implements an observer pattern that's freely attachable to any object");
MODULE_VERSION(0, 4, 2);
MODULE_BCVERSION(0, 1, 1);
MODULE_NODEPS;

static void freeEventListenerQueue(void *queue_p);
static void freeEventSubject(void *subject_p);
static int compareEventListeners(const void *first_p, const void *second_p, void *data);

/**
 * A GHashTable that maps objects to other GHashTables which themselves map string identifiers to EventListenerEntry objects with event listeners
 */
static GHashTable *subjects;

/**
 * Mutex to make the event module thread-safe
 */
static GRecMutex mutex;

MODULE_INIT
{
	subjects = g_hash_table_new_full(&g_direct_hash, &g_direct_equal, NULL, &freeEventSubject);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(subjects);
}

API void attachEventListener(void *subject, const char *event, void *custom, EventListener *listener)
{
	attachEventListenerWithPriority(subject, event, EVENT_LISTENER_PRIORITY_NORMAL, custom, listener);
}

API void attachEventListenerWithPriority(void *subject, const char *event, int priority, void *custom, EventListener *listener)
{
	g_rec_mutex_lock(&mutex);

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) == NULL) { // Create events if it doesn't exist yet
		events = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeEventListenerQueue);
		g_hash_table_insert(subjects, subject, events);
	}

	GQueue *queue;

	if((queue = g_hash_table_lookup(events, event)) == NULL) { // Create queue if it doesn't exist yet
		queue = g_queue_new();
		g_hash_table_insert(events, strdup(event), queue);
	}

	EventListenerEntry *entry = ALLOCATE_OBJECT(EventListenerEntry);
	entry->listener = listener;
	entry->custom = custom;
	entry->priority = priority;

	switch(priority) {
		case EVENT_LISTENER_PRIORITY_LOWEST:
			g_queue_push_head(queue, entry);
		break;
		case EVENT_LISTENER_PRIORITY_HIGHEST:
			g_queue_push_tail(queue, entry);
		break;
		default:
			g_queue_insert_sorted(queue, entry, &compareEventListeners, NULL);
		break;
	}

	triggerEvent(subject, "listener_attached", event);

	g_rec_mutex_unlock(&mutex);
}

API void detachEventListener(void *subject, const char *event, void *custom, EventListener *listener)
{
	g_rec_mutex_lock(&mutex);

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) != NULL) {
		GQueue *queue;

		if((queue = g_hash_table_lookup(events, event)) != NULL) {
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

					triggerEvent(subject, "listener_detached", event);

					break;
				}
			}
		}
	}

	g_rec_mutex_unlock(&mutex);
}

API int triggerEvent(void *subject, const char *event, ...)
{
	g_rec_mutex_lock(&mutex);

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) == NULL) {
		g_rec_mutex_unlock(&mutex);
		return -1;
	}

	GQueue *queue;

	if((queue = g_hash_table_lookup(events, event)) == NULL) {
		g_rec_mutex_unlock(&mutex);
		return -1;
	}

	// copy the list to make the event system reentrant
	GList *list = g_list_copy(queue->head);

	int counter = 0;

	for(GList *iter = list; iter != NULL; iter = iter->next, counter++) {
		va_list args;
		// Get args and listener
		va_start(args, event);
		EventListenerEntry *entry = iter->data;

		// Notify listener
		entry->listener(subject, event, entry->custom, args);
	}

	g_list_free(list);

	g_rec_mutex_unlock(&mutex);

	return counter;
}

API int getEventListenerCount(void *subject, const char *event)
{
	g_rec_mutex_lock(&mutex);

	GHashTable *events;

	if((events = g_hash_table_lookup(subjects, subject)) == NULL) { // Create events if it doesn't exist yet
		g_rec_mutex_unlock(&mutex);
		return 0;
	}

	GQueue *queue;

	if((queue = g_hash_table_lookup(events, event)) == NULL) { // Create queue if it doesn't exist yet
		g_rec_mutex_unlock(&mutex);
		return 0;
	}

	unsigned int length = g_queue_get_length(queue);

	g_rec_mutex_unlock(&mutex);

	return length;
}

/**
 * GCompareDataFunc to compare two EventListenerEntry objects by their priority
 *
 * @param first_p		the first EventListenerEntry
 * @param second_p		the second EventListenerEntry
 * @param data			unused
 */
static int compareEventListeners(const void *first_p, const void *second_p, void *data)
{
	const EventListenerEntry *first = first_p;
	const EventListenerEntry *second = second_p;

	return first->priority - second->priority;
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
