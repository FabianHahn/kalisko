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
typedef void (TimerCallback)(GTimeVal time);

API void initTimers();
API void freeTimers();
API GTimeVal *addTimer(GTimeVal time, TimerCallback *callback);
API bool delTimer(GTimeVal *time);
API void addTimeout(int timeout, TimerCallback *callback);
API GTimeVal getNextTimerTime();
API int getCurrentSleepTime();
API void notifyTimerCallbacks();
API bool hasMoreTimerCallbacks();
API void exitGracefully();
API bool isExiting();

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
#define TIMER_CALLBACK(CALLBACK) static void TIMER_CALLBACK_NAME(CALLBACK)(GTimeVal time)

#ifdef DLL_API_IMPORT

/**
 * @see addTimer
 * @param TIME			the time when the callback should be executed
 * @param CALLBACK		the callback to schedule
 */
#define TIMER_ADD(TIME, CALLBACK) $$(void, addTimer)((TIME), &TIMER_CALLBACK_NAME(CALLBACK))

/**
 * @see addTimeout
 * @param TIME			the timeout in microseconds after which the callback should be executed
 * @param CALLBACK		the callback to schedule
 */
#define TIMER_ADD_TIMEOUT(TIMEOUT, CALLBACK) $$(void, addTimeout)((TIMEOUT), &TIMER_CALLBACK_NAME(CALLBACK))

#endif

#endif
