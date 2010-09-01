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


#include <stdio.h> // vsnprintf
#include <stdarg.h> // va_list, va_start
#include <time.h> // time_t
#include <glib.h>

#include "api.h"
#include "log.h"

static void handleGlibLogMessage(const char *domain, GLogLevelFlags logLevel, const char *message, void *userData);

/**
 * Inits logging
 */
API void initLog()
{
	addHook("log");

	g_log_set_default_handler(handleGlibLogMessage, NULL);
}

/**
 * Creates a new log message and distribute it over the hook "log".
 *
 * @param type		the type of the log message
 * @param message	printf-like message to log
 */
API void logMessage(LogType type, char *message, ...)
{
	va_list va;
	char buffer[LOG_MSG_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, LOG_MSG_MAXLEN, message, va);

	triggerHook("log", type, buffer);
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
			logMessage(LOG_TYPE_ERROR, "%s: CRITICAL: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_ERROR:
			logMessage(LOG_TYPE_ERROR, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_WARNING:
			logMessage(LOG_TYPE_WARNING, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_INFO:
			logMessage(LOG_TYPE_INFO, "%s: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_MESSAGE:
			logMessage(LOG_TYPE_ERROR, "%s: MESSAGE: %s", fixedDomain, message);
			break;
		case G_LOG_LEVEL_DEBUG:
			logMessage(LOG_TYPE_DEBUG, "%s: %s", fixedDomain, message);
			break;
		default:
			logMessage(LOG_TYPE_WARNING, "Unknown '%s' log type: %d, message: '%s'", fixedDomain, logLevel, message);
	}
}
