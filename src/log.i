/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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
 * Log level enum describing possible logging modes
 */
typedef enum
{
	/** Debugging information that may be extremely verbose. Not for general use! */
	LOG_LEVEL_TRACE = 1,
	/** Unimportant verbose information */
	LOG_LEVEL_INFO = 2,
	/** Informational messages that describe the runtime state */
	LOG_LEVEL_NOTICE = 4,
	/** The function has an unexpected state but can go on with the work */
	LOG_LEVEL_WARNING = 8,
	/** The function has an unexpected state and can not end the work */
	LOG_LEVEL_ERROR = 16,
	/** No logging */
	LOG_LEVEL_NONE = 0,
	/** Log warnings and up */
	LOG_LEVEL_WARNING_UP = 24,
	/** Log infos and up */
	LOG_LEVEL_NOTICE_UP = 28,
	/** Log notices and up */
	LOG_LEVEL_INFO_UP = 30,
	/** Log everything! The sky is the limit! */
	LOG_LEVEL_ALL = 31,
} LogLevel;

/**
 * Log handler function pointer type
 */
typedef void (LogHandler)(const char *module, LogLevel level, const char *message);

/**
 * Inits logging
 *
 * @param level		the log level to use for the default log handler
 */
API void initLog(LogLevel level);

/**
 * Sets or resets the log handler
 *
 * @param handler		the new log handler to use or NULL if the default handler should be restored
 */
API void setLogHandler(LogHandler *handler);

/**
 * Determines whether (under the current settings) the specified level should get logged
 *
 * @param level     the level to query
 * @result          true if the log level should be displayed
 */
API bool shouldLog(LogLevel level);

/**
 * Formats a log message in a standard way. Meant for use in client implementing their own logging callback
 *
 * @param source        a source describing where the message came from (usually a module name)
 * @param level         the log level of the message to be logged
 * @param message       the original logging message
 * @result              a string containing the formatted message. The caller is responsible for free'ing the result.
 */
API char *formatLogMessage(const char *source, LogLevel level, const char *message);

/**
 * Creates a new log message and distribute it over the hook "log".
 *
 * @param module	the module in which the log message occurs
 * @param level		the level of the log message
 * @param message	printf-like message to log
 */
API void logMessage(const char *module, LogLevel level, const char *message, ...) G_GNUC_PRINTF(3, 4);

/**
 * Returns a static string with the name of a given log level
 *
 * @param level		the log level for which to retrieve the name
 * @result			the name of the log level
 */
API const char *getStaticLogLevelName(LogLevel level);

/**
 * Logs a message as trace debug information
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#ifdef TRACE
#define logTrace(...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define logTrace(...) ((void) 0)
#endif

/**
 * Logs a message as an notice
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define logNotice(...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_NOTICE, __VA_ARGS__)

/**
 * Logs a message as a info
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define logInfo(...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_INFO, __VA_ARGS__)

/**
 * Logs a message as a warning.
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define logWarning(...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_WARNING, __VA_ARGS__)

/**
 * Logs a message as an error.
 *
 * @see logMessage
 * @param ...	printf-like message to log
 */
#define logError(...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_ERROR, __VA_ARGS__)

/**
 * Logs a system error (strerror).
 *
 * @see logMessage
 * @param MESSAGE	printf-like message to log, the strerror result will be added automatically
 */
#define logSystemError(MESSAGE, ...) logMessage(STR(KALISKO_MODULE), LOG_LEVEL_ERROR, MESSAGE ": %s" , ##__VA_ARGS__, strerror(errno))

#endif
