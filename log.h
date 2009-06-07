/**
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Kalisko Developers nor the names of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LOG_H
#define LOG_H

#include <errno.h> // errno
#include <string.h> // strerror
#include <stdarg.h> // __VA_ARGS__
#include <time.h> // time
#include "types.h"

/**
 * Enumeration of the four standard log levels.
 */
typedef enum
{
	LOG_DEBUG, /** Information needed for debugging function. */
	LOG_INFO,  /** Verbose information what a function do. */
	LOG_WARNING, /** The function has an unexpected state but can go on with the work. */
	LOG_ERROR /** The function has an unexpected state and can not end the work. */
} LogType;

API void initLog();
API void logMessage(time_t time, LogType type, char *message, ...);

#define logSystemError(MESSAGE, ...) logMessage(time(NULL), LOG_ERROR, "%s: " MESSAGE, strerror(errno), ##__VA_ARGS__)
#define logError(...) logMessage(time(NULL), LOG_ERROR, __VA_ARGS__);
#define logWarning(...) logMessage(time(NULL), LOG_WARNING, __VA_ARGS__);
#define logInfo(...) logMessage(time(NULL), LOG_INFO, __VA_ARGS__);
#define logDebug(...) logMessage(time(NULL), LOG_DEBUG, __VA_ARGS__);

#define BUF 4096

#endif
