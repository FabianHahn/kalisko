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

static bool findFirstEntry(void *key, void *value, void *data);

typedef struct
{
	GTimeVal *time;
	TimerCallback *callback;
	void *custom_data;
} TimerEntry;

/**
 * Tree structure that organizes timer callbacks as a priority queue
 */
static GTree *timers;

/**
 * Initializes the timers
 */
API void initTimers()
{
	timers = g_tree_new_full(&compareTimes, NULL, &free, &free);
}

/**
 * Frees the timers
 */
API void freeTimers()
{
	exitGracefully();
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
	if(timers == NULL) {
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
	if(timers == NULL) {
		return false;
	}

	TimerEntry *entry = g_tree_lookup(timers, time);

	if(entry == NULL) { // don't continue if the entry doesn't exist
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
 * @result		time of the callback scheduled next, undefined if there are no callbacks
 */
API GTimeVal getNextTimerTime()
{
	GTimeVal nextTime;

	if(timers != NULL) {
		TimerEntry *entry = NULL;
		g_tree_foreach(timers, &findFirstEntry, &entry);

		if(entry != NULL) {
			nextTime = *entry->time;
		}
	}

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
	GTimeVal time;
	g_get_current_time(&time);

	while(hasMoreTimerCallbacks()) {
		TimerEntry *entry = NULL;
		g_tree_foreach(timers, &findFirstEntry, &entry);

		if(entry == NULL) { // Just to be sure, this should never happen, though
			break;
		}

		if(compareTimes(&time, entry->time, NULL) >= 0) { // This timer is ready, we can process it
			// Backup values
			GTimeVal entryTime = *entry->time;
			TimerCallback *callback = entry->callback;
			void *custom_data = entry->custom_data;

			// Remove it from the tree first
			g_tree_remove(timers, entry->time);

			// Now notify the callback
			callback(entryTime, custom_data);
		} else { // If this timer isn't ready, all further timers aren't ready either since they're organized in a priority queue
			break; // We can safely break now
		}
	}
}

/**
 * Checks if there are more scheduled timer callbacks
 *
 * @result	true if there are more scheduled timer callbacks
 */
API bool hasMoreTimerCallbacks()
{
	if(timers != NULL) {
		return g_tree_nnodes(timers) > 0;
	} else {
		return false;
	}
}

/**
 * Requests a graceful exit. This means that no more timer entries are scheduled and the program will exit if all remaining timers are processed.
 */
API void exitGracefully()
{
	if(timers != NULL) {
		g_tree_destroy(timers);
	}

	timers = NULL;
}

/**
 * Checks if the framework is exiting
 * @result 		true if exiting
 */
API bool isExiting()
{
	return timers == NULL;
}

/**
 * A GTraverseFunc to read the smallest tree entry and return immediately
 * @param key		the key of the node
 * @param value		the value of the node, unused
 * @param data		a container into which the pointer to the smallest tree entry should be written
 * @result			true if traversal should abort
 */
static bool findFirstEntry(void *key, void *value, void *data)
{
	TimerEntry **target = (TimerEntry **) data;
	*target = (TimerEntry *) value;

	return TRUE;
}
