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

static void processReadyCallbacks(void *item, void *data);
static gboolean assembleReadyCallbacks(void *key, void *value, void *data);
static gboolean findFirstTime(void *key, void *value, void *data);

typedef struct {
	GTimeVal *time;
	TimerCallback *callback;
} TimerEntry;

static GTree *timers;
bool blockTimers;

/**
 * Initializes the timers
 */
API void initTimers()
{
	timers = g_tree_new_full(&compareIntegers, NULL, &free, NULL);
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
 * @param time		the time when the callback should be executed
 * @param callback	the callback that should be called at the specified time
 */
API void addTimer(GTimeVal time, TimerCallback *callback)
{
	if(blockTimers) {
		return;
	}

	GTimeVal *timeContainer = allocateMemory(sizeof(GTimeVal));
	*timeContainer = time;

	while(g_tree_lookup(timers, timeContainer) != NULL) { // Find a free tree position
		timeContainer->tv_usec++;
	}

	g_tree_insert(timers, timeContainer, callback);
}

/**
 * Adds a timer callback after a specific timeout
 * @param timeout	the timeout from now after which the timer should be executed in microseconds
 * @param callback	the callback that should be called after the time elapsed
 */
API void addTimeout(int timeout, TimerCallback *callback)
{
	GTimeVal time;
	g_get_current_time(&time);
	g_time_val_add(&time, timeout);
	addTimer(time, callback);
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
	return sleepTime;
}

/**
 * Notifies all timer callbacks ready for execution
 */
API void notifyTimerCallbacks()
{
	GQueue *queue = g_queue_new();
	g_tree_foreach(timers, &assembleReadyCallbacks, queue);
	g_queue_foreach(queue, &processReadyCallbacks, NULL);
	g_queue_free(queue);
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
 * @param item	the item that's currently traversed
 * @param data	unused
 */
static void processReadyCallbacks(void *item, void *data)
{
	TimerEntry *entry = (TimerEntry *) item;
	entry->callback(*entry->time); // notify the callback

	// Cleanup
	g_tree_remove(timers, entry->time); // remove the entry from the tree
	free(entry);
}

/**
 * A GTraverseFunc to assemble all callbacks ready for execution
 * @param key	the key of the node
 * @param value	the value of the node
 * @param data	the queue to assemble the ready callbacks
 * @result		true if traversal should abort
 */
static gboolean assembleReadyCallbacks(void *key, void *value, void *data)
{
	GTimeVal *nodeTime = (GTimeVal *) key;
	TimerCallback *callback = (TimerCallback *) value;
	GQueue *queue = (GQueue *) data;

	GTimeVal time;
	g_get_current_time(&time);

	if(compareTimes(&time, nodeTime, NULL) > 0) { // if node time is greater or equal
		TimerEntry *entry = allocateMemory(sizeof(TimerEntry));
		entry->time = nodeTime;
		entry->callback = callback;
		g_queue_push_tail(queue, entry);
	} else {
		return TRUE; // abort
	}

	return FALSE;
}

/**
 * A GTraverseFunc to read the smallest tree entry and return immediately
 * @param key	the key of the node
 * @param value	the value of the node
 * @param data	unused
 * @result		true if traversal should abort
 */
static gboolean findFirstTime(void *key, void *value, void *data)
{
	GTimeVal *timeContainer = (GTimeVal *) data;
	GTimeVal *time = (GTimeVal *) key;
	*timeContainer = *time;

	return TRUE;
}
