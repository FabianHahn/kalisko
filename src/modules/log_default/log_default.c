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
#include <time.h> //time_t, strftime
#include <glib.h>

#include "dll.h"
#include "hooks.h"
#include "modules/time_util/time_util.h"

#include "log.h"
#include "api.h"

HOOK_LISTENER(log);

API bool module_init()
{
	return HOOK_ATTACH(log, log);
}

API void module_finalize()
{
	HOOK_DETACH(log, log);
}

API GList *module_depends()
{
	return g_list_append(NULL, "time_util");
}

/**
 * Log message listener to write them into stderr.
 */
HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	char *dateTime = $(char *, time_util, getCurrentDateTimeString)();

	switch(type) {
		case LOG_TYPE_ERROR:
			fprintf(stderr, "%s ERROR: %s\n", dateTime, message);
		break;
		case LOG_TYPE_WARNING:
			fprintf(stderr, "%s WARNING: %s\n", dateTime, message);
		break;
		case LOG_TYPE_INFO:
			fprintf(stderr, "%s INFO: %s\n", dateTime, message);
		break;
		case LOG_TYPE_DEBUG:
			fprintf(stderr, "%s DEBUG: %s\n", dateTime, message);
		break;
	}

	free(dateTime);
	fflush(stderr);
}
