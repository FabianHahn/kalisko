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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "api.h"
#include "hooks.h"
#include "types.h"
#include "memory_alloc.h"

static void addHookStatsEntry(void *key, void *value, void *data);
static void freeHookListenerList(void *list_p);

static GThread *mainthread;
static GHashTable *hooks;

/**
 * Initializes the hooks data structures
 */
API void initHooks()
{
	mainthread = g_thread_self();
	hooks = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeHookListenerList);
}

/**
 * Frees the hooks data structures
 */
API void freeHooks()
{
	// Destroy hash table struct as well
	g_hash_table_destroy(hooks);
}

/**
 * Adds a new hook
 *
 * @param hook_name		the name of the hook
 * @result				true if successful
 */
API bool addHook(char *hook_name)
{
	if(g_thread_self() != mainthread) {
		return false;
	}

	if(g_hash_table_lookup(hooks, hook_name) != NULL) { // A hook with that name already exists
		return false;
	}

	// Insert an empty list
	g_hash_table_insert(hooks, strdup(hook_name), g_queue_new());

	return true;
}

/**
 * Deletes an existing hook with all its listeners
 *
 * @param hook_name		the name of the hook
 * @result				true if successful
 */
API bool delHook(char *hook_name)
{
	if(g_thread_self() != mainthread) {
		return false;
	}

	return g_hash_table_remove(hooks, hook_name);
}

/**
 * Attaches a listener to a hook
 *
 * @param hook_name		the name of the hook
 * @param listener		the hook listener to attach
 * @param custom_data	custom data to pass to the hook listener when triggered
 * @result				true if successful
 */
API bool attachToHook(char *hook_name, HookListener *listener, void *custom_data)
{
	if(g_thread_self() != mainthread) {
		return false;
	}

	GQueue *hook;

	if((hook = g_hash_table_lookup(hooks, hook_name)) == NULL) { // a hook with that name doesn't exist
		return false;
	}

	HookListenerEntry *entry = allocateMemory(sizeof(HookListenerEntry));

	entry->listener = listener;
	entry->custom_data = custom_data;

	g_queue_push_tail(hook, entry);

	return true;
}

/**
 * Detaches a listener from a hook
 *
 * @param hook_name		the name of the hook
 * @param listener		the hook listener to attach
 * @param custom_data	custom data passed to the hook listener when called
 * @result				true if successful
 */
API bool detachFromHook(char *hook_name, HookListener *listener, void *custom_data)
{
	if(g_thread_self() != mainthread) {
		return false;
	}

	GQueue *hook;

	if((hook = g_hash_table_lookup(hooks, hook_name)) == NULL) { // a hook with that name doesn't exist
		return false;
	}

	for(GList *iter = hook->head; iter != NULL; iter = iter->next) {
		HookListenerEntry *entry = iter->data;

		if(entry->listener == listener && entry->custom_data == custom_data) { // Is this the right listener entry?
			g_queue_remove(hook, entry);
			free(entry);

			return true;
		}
	}

	return false;
}

/**
 * Triggers a hook and notifies all its listeners
 *
 * @param hook_name		the name of the hook
 * @param ...			the data to pass to the listeners
 * @result				the number of listeners notified, -1 if hook not found
 */
API int triggerHook(char *hook_name, ...)
{
	if(g_thread_self() != mainthread) {
		return 0;
	}

	GQueue *hook;

	if((hook = g_hash_table_lookup(hooks, hook_name)) == NULL) {
		return -1;
	}

	va_list args;
	HookListenerEntry *entry;
	HookListener *listener;
	int counter = 0;

	for(GList *iter = hook->head; iter != NULL; iter = iter->next, counter++) {
		// Get args and listener
		va_start(args, hook_name);
		entry = iter->data;
		listener = entry->listener;

		// Notify listener
		listener(hook_name, entry->custom_data, args);
	}

	return counter;
}

/**
 * Retrieves statistics about hooks
 *
 * @see freeHookStats
 * @return					a list of HookStatsEntry structs, must be freed with freeHookStats
 */
API GQueue *getHookStats()
{
	if(g_thread_self() != mainthread) {
		return NULL;
	}

	GQueue *result = g_queue_new();

	g_hash_table_foreach(hooks, &addHookStatsEntry, result);

	return result;
}

/**
 * Frees hook stats retrieved by getHookStats
 *
 * @see getHookStats
 * @param hook_stats		a list of HookStatsEntry structs
 */
API void freeHookStats(GQueue *hook_stats)
{
	for(GList *iter = hook_stats->head; iter != NULL; iter = iter->next) {
		free(iter->data);
	}

	g_queue_free(hook_stats);
}

/**
 * A GHFunc to add a HookStatsEntry
 *
 * @see getHookStats
 * @param key			the name of the hook
 * @param value			the hook's listener list
 * @param data			a pointer to the stats list
 */
static void addHookStatsEntry(void *key, void *value, void *data)
{
	char *name = key;
	GQueue *listeners = value;
	GQueue *list = data;

	HookStatsEntry *entry = allocateMemory(sizeof(HookStatsEntry));
	entry->hook_name = name;
	entry->num_listeners = g_queue_get_length(listeners);

	g_queue_push_tail(list, entry);
}

/**
 * A GDestroyNotify function to free a hook listener list
 *
 * @param list_p			a pointer to the list to frees
 */
static void freeHookListenerList(void *list_p)
{
	GQueue *list = list_p;

	for(GList *iter = list->head; iter != NULL; iter = iter->next) {
		free(iter->data);
	}

	g_queue_free(list);
}

