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


#include <stdlib.h>
#include <stdio.h> // vsnprintf
#include <stdarg.h> // va_list, va_start
#include <time.h> // time_t
#include <glib.h>

#include "api.h"
#include "log.h"

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_TYPE_DEBUG
#endif

static void handleGlibLogMessage(const char *domain, GLogLevelFlags logLevel, const char *message, void *userData);
static void defaultLogHandler(const char *name, LogType type, const char *message);

static LogHandler *logHandler = &defaultLogHandler;

/**
 * Inits logging
 */
API void initLog()
{
	g_log_set_default_handler(handleGlibLogMessage, NULL);
}

/**
 * Sets or resets the log handler
 *
 * @param handler		the new log handler to use or NULL if the default handler should be restored
 */
API void setLogHandler(LogHandler *handler)
{
	if(handler == NULL) {
		logHandler = &defaultLogHandler;
	} else {
		logHandler = handler;
	}
}

/**
 * Creates a new log message and distribute it over the hook "log".
 *
 * @param module	the module in which the log message occurs
 * @param type		the type of the log message
 * @param message	printf-like message to log
 */
API void logMessage(const char *module, LogType type, const char *message, ...)
{
	va_list va;
	char buffer[LOG_MSG_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, LOG_MSG_MAXLEN, message, va);

	logHandler(module, type, buffer);
}

/**
 * Handles GLib log messages and put them into Kalisko's own log system.
 *
 * @param domain
 * @param logLevel
 * @param message
 * @param userData
 */
static void handleGlibLogMessage(const char *domain, GLogLevelFlags logLevel, const char *message, void *userData)
{
	const char *fixedDomain = (domain == NULL ? "GLib default" : domain);

	switch(logLevel) {
		case G_LOG_LEVEL_CRITICAL:
			logMessage("glib", LOG_TYPE_ERROR, "%s: CRITICAL: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_ERROR:
			logMessage("glib", LOG_TYPE_ERROR, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_WARNING:
			logMessage("glib", LOG_TYPE_WARNING, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_INFO:
			logMessage("glib", LOG_TYPE_INFO, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_MESSAGE:
			logMessage("glib", LOG_TYPE_ERROR, "%s: MESSAGE: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_DEBUG:
			logMessage("glib", LOG_TYPE_DEBUG, "%s: %s", fixedDomain, message);
			break;
		default:
			logMessage("glib", LOG_TYPE_WARNING, "Unknown '%s' log type: %d, message: '%s'", fixedDomain, logLevel, message);
	}
}

static void defaultLogHandler(const char *name, LogType type, const char *message)
{
	GTimeVal now;
	g_get_current_time(&now);
	char *dateTime = g_time_val_to_iso8601(&now);

	switch(LOG_DEFAULT_LEVEL) {
		case LOG_TYPE_DEBUG:
			if(type == LOG_TYPE_DEBUG)
				fprintf(stderr, "%s [%s] DEBUG: %s\n", dateTime, name, message);
		case LOG_TYPE_INFO:
			if(type == LOG_TYPE_INFO)
				fprintf(stderr, "%s [%s] INFO: %s\n", dateTime, name, message);
		case LOG_TYPE_WARNING:
			if(type == LOG_TYPE_WARNING)
				fprintf(stderr, "%s [%s] WARNING: %s\n", dateTime, name, message);
		case LOG_TYPE_ERROR:
			if(type == LOG_TYPE_ERROR)
				fprintf(stderr, "%s [%s] ERROR: %s\n", dateTime, name, message);
	}

	free(dateTime);
	fflush(stderr);
}
