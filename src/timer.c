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
#include "api.h"
#include "timer.h"
#include "util.h"
#include "memory_alloc.h"
#include "types.h"

static bool assembleReadyCallbacks(void *time_p, void *entry_p, void *queue_p);
static bool assemblePendingCallbacks(void *time_p, void *entry_p, void *queue_p);
static bool findFirstTime(void *key, void *value, void *data);

typedef struct
{
	GTimeVal *time;
	TimerCallback *callback;
	void *custom_data;
	/** false if the entry can be safely removed */
	bool processing;
} TimerEntry;

/**
 * Tree structure that organizes timer callbacks as a priority queue
 */
static GTree *timers;

bool blockTimers;

/**
 * Initializes the timers
 */
API void initTimers()
{
	timers = g_tree_new_full(&compareTimes, NULL, &free, &free);
	blockTimers = false;
}

/**
 * Frees the timers
 */
API void freeTimers()
{
	blockTimers = true;
	g_tree_destroy(timers);
}

/**
 * Adds a timer callback
 * @param time				the time when the callback should be executed
 * @param callback			the callback that should be called at the specified time
 * @param custom_data		custom data to be passed to the timer callback on execution
 * @result 					a pointer to the actual time scheduled
 */
API GTimeVal *addTimer(GTimeVal time, TimerCallback *callback, void *custom_data)
{
	if(blockTimers) {
		return NULL;
	}

	GTimeVal *timeContainer = allocateMemory(sizeof(GTimeVal));
	*timeContainer = time;

	while(g_tree_lookup(timers, timeContainer) != NULL) { // Find a free tree position
		timeContainer->tv_usec++;
	}

	TimerEntry *entry = allocateMemory(sizeof(TimerEntry));
	entry->time = timeContainer;
	entry->callback = callback;
	entry->custom_data = custom_data;
	entry->processing = false;
	g_tree_insert(timers, timeContainer, entry);

	return timeContainer;
}

/**
 * Removes a timer callback
 * @param time		the time of the callback to delete
 * @result			true if successful
 */
API bool delTimer(GTimeVal *time)
{
	TimerEntry *entry = g_tree_lookup(timers, time);

	if(entry == NULL || entry->processing) { // don't continue if the entry doesn't exist or is already processing
		return false;
	}

	return g_tree_remove(timers, time);
}

/**
 * Adds a timer callback after a specific timeout
 * @param timeout			the timeout from now after which the timer should be executed in microseconds
 * @param callback			the callback that should be called after the time elapsed
 * @param custom_data		custom data to be passed to the timer callback on execution
 */
API GTimeVal *addTimeout(int timeout, TimerCallback *callback, void *custom_data)
{
	GTimeVal time;
	g_get_current_time(&time);
	g_time_val_add(&time, timeout);
	return addTimer(time, callback, custom_data);
}

/**
 * Returns the time of the callback scheduled next
 * @result		time of the callback scheduled next
 */
API GTimeVal getNextTimerTime()
{
	GTimeVal nextTime;
	g_get_current_time(&nextTime);
	g_tree_foreach(timers, &findFirstTime, &nextTime);
	return nextTime;
}

/**
 * Returns the time in microseconds to sleep until the next callback is scheduled
 * @result		time in microseconds to sleep until the next scheduled callback
 */
API int getCurrentSleepTime()
{
	GTimeVal time;
	g_get_current_time(&time);
	GTimeVal nextTime = getNextTimerTime();

	int sleepTime = (nextTime.tv_sec - time.tv_sec) * G_USEC_PER_SEC;
	sleepTime += (nextTime.tv_usec - time.tv_usec);

	return sleepTime > 0 ? sleepTime : 0;
}

/**
 * Notifies all timer callbacks ready for execution
 */
API void notifyTimerCallbacks()
{
	GQueue *processing = g_queue_new();
	g_tree_foreach(timers, &assembleReadyCallbacks, processing);

	// Traverse the processing list and notify all callbacks in it
	for(GList *iter = processing->head; iter != NULL; iter = iter->next) {
		TimerEntry *entry = iter->data;
		entry->callback(*entry->time, entry->custom_data); // notify the callback
		g_tree_remove(timers, entry->time); // remove the entry from the tree
	}

	g_queue_free(processing);
}

/**
 * Checks if there are more scheduled timer callbacks
 * @result	true if there are more scheduled timer callbacks
 */
API bool hasMoreTimerCallbacks()
{
	return g_tree_nnodes(timers) > 0;
}

/**
 * Requests a graceful exit. This means that no more timer entries are scheduled and the program will exit if all remaining timers are processed.
 */
API void exitGracefully()
{
	// Free all timers that can't be processing right now
	GQueue *pending = g_queue_new();
	g_tree_foreach(timers, &assemblePendingCallbacks, pending);

	// Traverse the pending list and notify all callbacks in it
	for(GList *iter = pending->head; iter != NULL; iter = iter->next) {
		TimerEntry *entry = iter->data;
		g_tree_remove(timers, entry->time); // remove the entry from the tree
	}

	g_queue_free(pending);

	// Block the timers so no more of them are scheduled
	blockTimers = true;
}

/**
 * Checks if the framework is exiting
 * @result 		true if exiting
 */
API bool isExiting()
{
	return blockTimers;
}

/**
 * A GTraverseFunc to assemble all callbacks ready for execution
 * @param time_p		a pointer to the time of the callback
 * @param entry_p		a pointer to the timer entry
 * @param queue_p			a pointer to the queue to assemble the ready callbacks in
 * @result				true if traversal should abort
 */
static bool assembleReadyCallbacks(void *time_p, void *entry_p, void *queue_p)
{
	GTimeVal *nodeTime = time_p;
	TimerEntry *entry = entry_p;
	GQueue *queue = queue_p;

	GTimeVal time;
	g_get_current_time(&time);

	if(compareTimes(&time, nodeTime, NULL) >= 0 && !entry->processing) { // if node time is greater or equal and not processing yet
		entry->processing = true; // set the entry to processing so it can't be removed anymore
		g_queue_push_tail(queue, entry);
	} else {
		return true; // abort
	}

	return false;
}

/**
 * A GTraverseFunc to assemble all pending callbacks not yet ready for execution
 * @param time_p		a pointer to the time of the callback, though unused
 * @param entry_p		a pointer to the timer entry
 * @param queue_p			a pointer to the queue to assemble the pending callbacks in
 * @result				true if traversal should abort
 */
static bool assemblePendingCallbacks(void *time_p, void *entry_p, void *queue_p)
{
	TimerEntry *entry = entry_p;
	GQueue *queue = queue_p;

	if(!entry->processing) {
		g_queue_push_tail(queue, entry);
	}

	return false;
}

/**
 * A GTraverseFunc to read the smallest tree entry and return immediately
 * @param key		the key of the node
 * @param value		the value of the node, unused
 * @param data		a container into which the smallest tree entry key should be written
 * @result			true if traversal should abort
 */
static bool findFirstTime(void *key, void *value, void *data)
{
	GTimeVal *timeContainer = (GTimeVal *) data;
	GTimeVal *time = (GTimeVal *) key;
	*timeContainer = *time;

	return TRUE;
}
