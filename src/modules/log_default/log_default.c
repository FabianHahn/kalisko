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
#include <stdio.h> // fprintf
#include <glib.h>

#include "dll.h"
#include "modules/event/event.h"
#include "log.h"

#include "api.h"

MODULE_NAME("log_default");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Kalisko's default log provider that's always loaded initially");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_log(void *subject, const char *event, void *data, va_list args);

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_TYPE_DEBUG
#endif

MODULE_INIT
{
	$(void, event, attachEventListener)(NULL, "log", NULL, &listener_log);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "log", NULL, &listener_log);
}

/**
 * Log message listener to write them into stderr.
 */
static void listener_log(void *subject, const char *event, void *data, va_list args)
{
	LogType type = va_arg(args, LogType);
	char *message = va_arg(args, char *);

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	switch(LOG_DEFAULT_LEVEL) {
		case LOG_TYPE_DEBUG:
			if(type == LOG_TYPE_DEBUG)
				fprintf(stderr, "%s DEBUG: %s\n", dateTime, message);
		case LOG_TYPE_INFO:
			if(type == LOG_TYPE_INFO)
				fprintf(stderr, "%s INFO: %s\n", dateTime, message);
		case LOG_TYPE_WARNING:
			if(type == LOG_TYPE_WARNING)
				fprintf(stderr, "%s WARNING: %s\n", dateTime, message);
		case LOG_TYPE_ERROR:
			if(type == LOG_TYPE_ERROR)
				fprintf(stderr, "%s ERROR: %s\n", dateTime, message);
	}

	free(now);
	free(dateTime);
	fflush(stderr);
}
