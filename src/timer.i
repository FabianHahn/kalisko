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

#ifndef TIMER_H
#define TIMER_H

#include <glib.h>
#include "types.h"

/**
 * Function pointer for timer callbacks
 */
typedef void (TimerCallback)(GTimeVal time, void *custom_data);


/**
 * Initializes the timers
 */
API void initTimers();

/**
 * Frees the timers
 */
API void freeTimers();

/**
 * Adds a timer callback
 * @param module			the module that registers this timer
 * @param time				the time when the callback should be executed
 * @param callback			the callback that should be called at the specified time
 * @param custom_data		custom data to be passed to the timer callback on execution
 * @result 					a pointer to the actual time scheduled
 */
API GTimeVal *addTimer(const char *module, GTimeVal time, TimerCallback *callback, void *custom_data);

/**
 * Removes a timer callback
 * @param time		the time of the callback to delete
 * @result			true if successful
 */
API bool delTimer(GTimeVal *time);

/**
 * Adds a timer callback after a specific timeout
 * @param module			the module that registers this timer
 * @param timeout			the timeout from now after which the timer should be executed in microseconds
 * @param callback			the callback that should be called after the time elapsed
 * @param custom_data		custom data to be passed to the timer callback on execution
 */
API GTimeVal *addTimeout(const char *module, int timeout, TimerCallback *callback, void *custom_data);

/**
 * Returns the time of the callback scheduled next
 * @result		time of the callback scheduled next, undefined if there are no callbacks
 */
API GTimeVal getNextTimerTime();

/**
 * Returns the time in microseconds to sleep until the next callback is scheduled
 * @result		time in microseconds to sleep until the next scheduled callback
 */
API int getCurrentSleepTime();

/**
 * Notifies all timer callbacks ready for execution
 */
API void notifyTimerCallbacks();

/**
 * Checks if there are more scheduled timer callbacks
 *
 * @result	true if there are more scheduled timer callbacks
 */
API bool hasMoreTimerCallbacks();

/**
 * Requests a graceful exit. This means that no more timer entries are scheduled and the program will exit if all remaining timers are processed.
 */
API void exitGracefully();

/**
 * Checks if the framework is exiting
 * @result 		true if exiting
 */
API bool isExiting();

/**
 * Removes all timers registered by a module
 *
 * @param module	the name of the module to remove all timers from
 * @result			the number of timers removed
 */
API int removeModuleTimers(const char *module);

// Wrapper macros for timers
/**
 * Transforms the given callback name to a general valid form.
 *
 * @param CALLBACK	name of the callback
 */
#define TIMER_CALLBACK_NAME(CALLBACK) _timer_callback_ ## CALLBACK

/**
 * Transforms the given callback name to a valid function head.
 *
 * @param CALLBACK	name of the callback. Must be unique in the scope of the sourcefile.
 */
#define TIMER_CALLBACK(CALLBACK) static void TIMER_CALLBACK_NAME(CALLBACK)(GTimeVal time, void *custom_data)

/**
 * @see addTimer
 * @param TIME			the time when the callback should be executed
 * @param CALLBACK		the callback to schedule
 */
#define TIMER_ADD(TIME, CALLBACK) addTimer(STR(KALISKO_MODULE), (TIME), &TIMER_CALLBACK_NAME(CALLBACK), NULL)

/**
 * @see addTimer
 * @param TIME			the time when the callback should be executed
 * @param CALLBACK		the callback to schedule
 * @param CDATA			custom data to be passed to the callback
 */
#define TIMER_ADD_EX(TIME, CALLBACK, CDATA) addTimer(STR(KALISKO_MODULE), (TIME), &TIMER_CALLBACK_NAME(CALLBACK), (CDATA))

/**
 * @see addTimeout
 * @param TIME			the timeout in microseconds after which the callback should be executed
 * @param CALLBACK		the callback to schedule
 */
#define TIMER_ADD_TIMEOUT(TIMEOUT, CALLBACK) addTimeout(STR(KALISKO_MODULE), (TIMEOUT), &TIMER_CALLBACK_NAME(CALLBACK), NULL)

/**
 * @see addTimeout
 * @param TIME			the timeout in microseconds after which the callback should be executed
 * @param CALLBACK		the callback to schedule
 * @param CDATA			custom data to be passed to the callback
 */
#define TIMER_ADD_TIMEOUT_EX(TIMEOUT, CALLBACK, CDATA) addTimeout(STR(KALISKO_MODULE), (TIMEOUT), &TIMER_CALLBACK_NAME(CALLBACK), (CDATA))

/**
 * @see delTimer
 * @param TIME			the scheduled time of the callback to be deleted
 */
#define TIMER_DEL(TIME) delTimer((TIME))

/**
 * Removes all timers registered by this module
 */
#define TIMERS_CLEAR removeModuleTimers(STR(KALISKO_MODULE))

#endif
