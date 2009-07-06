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
#include "api.h"
#include "hooks.h"
#include "types.h"
#include "memory_alloc.h"

static void addHookStatsEntry(void *key, void *value, void *data);
static gboolean freeList(void *hook_name, void *list, void *user_data);

static GHashTable *hooks;

/**
 * Initializes the hooks data structures
 */
API void initHooks()
{
	hooks = g_hash_table_new(&g_str_hash, &g_str_equal);
}

/**
 * Frees the hooks data structures
 */
API void freeHooks()
{
	// Remove all keys and free them
	g_hash_table_foreach_remove(hooks, &freeList, NULL);

	// Destroy hash table struct as well
	g_hash_table_destroy(hooks);
}

/**
 * Adds a new hook
 *
 * @param hook_name		the name of the hook
 * @result				true if successful
 */
API bool addHook(const char *hook_name)
{
	if(g_hash_table_lookup_extended(hooks, hook_name, NULL, NULL)) { // A hook with that name already exists
		return false;
	}

	// Insert an empty list
	g_hash_table_insert(hooks, (void *) hook_name, NULL);

	return true;
}

/**
 * Deletes an existing hook with all its listeners
 *
 * @param hook_name		the name of the hook
 * @result				true if successful
 */
API bool delHook(const char *hook_name)
{
	GList *hook;

	if(!g_hash_table_lookup_extended(hooks, hook_name, NULL, (void **) &hook)) { // A hook with that name doesn't exist
		return false;
	}

	// Free the hook's listener list
	g_list_free(hook);

	// Remove the hook from the hooks table
	g_hash_table_remove(hooks, hook_name);

	return true;
}

/**
 * Attaches a listener to a hook
 *
 * @param hook_name		the name of the hook
 * @param listener		the hook listener to attach
 * @param custom_data	custom data to pass to the hook listener when triggered
 * @result				true if successful
 */
API bool attachToHook(const char *hook_name, HookListener *listener, void *custom_data)
{
	GList *hook;
	char *hook_own_name;

	if(!g_hash_table_lookup_extended(hooks, hook_name, (void **) &hook_own_name, (void **) &hook)) { // A hook with that name doesn't exist
		return false;
	}

	HookListenerEntry *entry = allocateObject(HookListenerEntry);

	entry->listener = listener;
	entry->custom_data = custom_data;

	// Prepend the new listener to the hook
	hook = g_list_prepend(hook, entry);

	// Update the hook entry in the hooks table
	g_hash_table_replace(hooks, (void *) hook_own_name, hook);

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
API bool detachFromHook(const char *hook_name, HookListener *listener, void *custom_data)
{
	GList *hook;
	char *hook_own_name;

	if(!g_hash_table_lookup_extended(hooks, hook_name, (void **) &hook_own_name, (void **) &hook)) { // A hook with that name doesn't exist
		return false;
	}

	HookListenerEntry *entry;

	for(; hook != NULL; hook = hook->next)
	{
		entry = hook->data;

		if(entry->listener == listener && entry->custom_data == custom_data) // Is this the right listener entry?
		{
			// Remove the listener entry from the list
			hook = g_list_remove(hook, entry);

			// Update the hook entry in the hooks table
			g_hash_table_replace(hooks, (void *) hook_own_name, hook);

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
API int triggerHook(const char *hook_name, ...)
{
	GList *hook;

	if(!g_hash_table_lookup_extended(hooks, hook_name, NULL, (void **) &hook)) { // A hook with that name doesn't exist
		return -1;
	}

	va_list args;
	HookListenerEntry *entry;
	HookListener *listener;
	int counter;

	for(counter = 0; hook != NULL; hook = hook->next, counter++)
	{
		// Get args and listener
		va_start(args, hook_name);
		entry = hook->data;

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
 * @return					a list of HookStatsEntry structs
 */
API GList *getHookStats()
{
	GList *result = NULL;

	g_hash_table_foreach(hooks, &addHookStatsEntry, &result);

	return result;
}

/**
 * Frees hook stats retrieved by getHookStats
 *
 * @see getHookStats
 * @param hook_stats		a list of HookStatsEntry structs
 */
API void freeHookStats(GList *hook_stats)
{
	for(GList *iter = hook_stats; iter != NULL; iter = iter->next) {
		free(iter->data);
	}

	g_list_free(hook_stats);
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
	GList *listeners = value;
	GList **list = data;

	HookStatsEntry *entry = allocateObject(HookStatsEntry);
	entry->hook_name = name;
	entry->num_listeners = g_list_length(listeners);

	*list = g_list_append(*list, entry);
}

/**
 * Helper function to free a list
 *
 * @param data			The list to free
 */
static gboolean freeList(void *hook_name, void *list, void *user_data)
{
	HookListenerEntry *entry;
	GList *hook = list;

	for(; hook != NULL; hook = hook->next)
	{
		entry = hook->data;

		free(entry);
	}

	g_list_free((GList *) list);

	return TRUE;
}


