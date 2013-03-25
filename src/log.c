/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h> // vsnprintf
#include <stdarg.h> // va_list, va_start
#include <time.h> // time_t
#include <glib.h>

#define API
#include "log.h"

static void handleGlibLogMessage(const char *domain, GLogLevelFlags logLevel, const char *message, void *userData);
static void defaultLogHandler(const char *name, LogLevel level, const char *message);

static LogLevel defaultLevel;
static LogHandler *logHandler = &defaultLogHandler;

API void initLog(LogLevel level)
{
	defaultLevel = level;
	g_log_set_default_handler(handleGlibLogMessage, NULL);
}

API bool shouldLog(LogLevel level)
{
	return defaultLevel & level;
}

API char *formatLogMessage(const char *source, LogLevel level, const char *message)
{
	GDateTime *now = g_date_time_new_now_local();
	GString *result = g_string_new("");

	g_string_append_printf(result, "[%02d:%02d:%02d] [%s:%s] %s", g_date_time_get_hour(now), g_date_time_get_minute(now), g_date_time_get_second(now), source, getStaticLogLevelName(level), message);

	g_date_time_unref(now);
	return g_string_free(result, false);
}

API void setLogHandler(LogHandler *handler)
{
	if(handler == NULL) {
		logHandler = &defaultLogHandler;
	} else {
		logHandler = handler;
	}
}

API void logMessage(const char *module, LogLevel level, const char *message, ...)
{
	va_list va;
	char buffer[LOG_MSG_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, LOG_MSG_MAXLEN, message, va);

	logHandler(module, level, buffer);
}

API const char *getStaticLogLevelName(LogLevel level)
{
	const char *name = "unknown";

	if(level & LOG_LEVEL_TRACE) {
		name = "trace";
	} else if(level & LOG_LEVEL_INFO) {
		name = "info";
	} else if(level & LOG_LEVEL_NOTICE) {
		name = "notice";
	} else if(level & LOG_LEVEL_WARNING) {
		name = "warning";
	} else if(level & LOG_LEVEL_ERROR) {
		name = "error";
	}

	return name;
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
			logMessage("glib", LOG_LEVEL_ERROR, "%s: CRITICAL: %s", fixedDomain, message);
		break;
		case G_LOG_LEVEL_ERROR:
			logMessage("glib", LOG_LEVEL_ERROR, "%s: %s", fixedDomain, message);
		break;
		case G_LOG_LEVEL_WARNING:
			logMessage("glib", LOG_LEVEL_WARNING, "%s: %s", fixedDomain, message);
		break;
		case G_LOG_LEVEL_MESSAGE:
			logMessage("glib", LOG_LEVEL_NOTICE, "%s: %s", fixedDomain, message);
		break;
		case G_LOG_LEVEL_INFO:
			logMessage("glib", LOG_LEVEL_INFO, "%s: %s", fixedDomain, message);
		break;
		case G_LOG_LEVEL_DEBUG:
			logMessage("glib", LOG_LEVEL_TRACE, "%s: %s", fixedDomain, message);
		break;
		default:
			logMessage("glib", LOG_LEVEL_WARNING, "Unknown '%s' log type: %d, message: '%s'", fixedDomain, logLevel, message);
		break;
	}
}

static void defaultLogHandler(const char *name, LogLevel level, const char *message)
{
	if(shouldLog(level)) {
		char *formatted = formatLogMessage(name, level, message);
		fprintf(stderr, "%s\n", formatted);
		free(formatted);
	}
	fflush(stderr);
}

