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
	/** Information needed for debugging function. */
	LOG_TYPE_DEBUG,
	/** Verbose information what a function do. */
	LOG_TYPE_INFO,
	/** The function has an unexpected state but can go on with the work. */
	LOG_TYPE_WARNING,
	/** The function has an unexpected state and can not end the work. */
	LOG_TYPE_ERROR
} LogType;

/**
 * Log handler function pointer type
 */
typedef void (LogHandler)(const char *module, LogType type, const char *message);


/**
 * Inits logging
 */
API void initLog();

/**
 * Sets or resets the log handler
 *
 * @param handler		the new log handler to use or NULL if the default handler should be restored
 */
API void setLogHandler(LogHandler *handler);

/**
 * Creates a new log message and distribute it over the hook "log".
 *
 * @param module	the module in which the log message occurs
 * @param type		the type of the log message
 * @param message	printf-like message to log
 */
API void logMessage(const char *module, LogType type, const char *message, ...) G_GNUC_PRINTF(3, 4);

/**
 * Logs a system error (strerror).
 *
 * @see logMessage
 * @param MESSAGE	printf-like message to log, the strerror result will be added automatically
 */
#define LOG_SYSTEM_ERROR(MESSAGE, ...) $$(void, logMessage)(STR(KALISKO_MODULE), LOG_TYPE_ERROR, MESSAGE ": %s" , ##__VA_ARGS__, strerror(errno))

/**
 * Logs a message as an error.
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define LOG_ERROR(...) $$(void, logMessage)(STR(KALISKO_MODULE), LOG_TYPE_ERROR, __VA_ARGS__);

/**
 * Logs a message as a warning.
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define LOG_WARNING(...) $$(void, logMessage)(STR(KALISKO_MODULE), LOG_TYPE_WARNING, __VA_ARGS__);

/**
 * Logs a message as an info.
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define LOG_INFO(...) $$(void, logMessage)(STR(KALISKO_MODULE), LOG_TYPE_INFO, __VA_ARGS__);

/**
 * Logs a message as a debug information
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define LOG_DEBUG(...) $$(void, logMessage)(STR(KALISKO_MODULE), LOG_TYPE_DEBUG, __VA_ARGS__);

#endif

/**
 * The maximal length for a log message.
 */
#define LOG_MSG_MAXLEN 4096

