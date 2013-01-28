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


#include <glib.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "modules/event/event.h"

#define API

MODULE_NAME("log_event");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The log_event module provides access to the Kalisko log system using a global event that clients can attach to");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 0));

static void listener_attached(void *subject, const char *event, void *data, va_list args);
static void listener_detached(void *subject, const char *event, void *data, va_list args);
static void eventLogHandler(const char *module, LogLevel level, const char *message);

static int count = 0;

MODULE_INIT
{
	$(void, event, attachEventListener)(NULL, "listener_attached", NULL, &listener_attached);
	$(void, event, attachEventListener)(NULL, "listener_detached", NULL, &listener_detached);

	count = $(int, event, getEventListenerCount)(NULL, "log");

	if(count > 0) {
		$$(void, setLogHandler)(&eventLogHandler); // set log handler
		logInfo("Log event handler installed");
	}

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "listener_attached", NULL, &listener_attached);
	$(void, event, detachEventListener)(NULL, "listener_detached", NULL, &listener_detached);

	if(count > 0) {
		$$(void, setLogHandler)(NULL); // restore log handler
		logInfo("Default log handler restored");
	}
}

static void listener_attached(void *subject, const char *event, void *data, va_list args)
{
	char *attached_event = va_arg(args, char *);

	if(g_strcmp0(attached_event, "log") == 0) {
		if(count == 0) {
			$$(void, setLogHandler)(&eventLogHandler); // set log handler
			logInfo("Log event handler installed");
		}

		count++;
	}
}

static void listener_detached(void *subject, const char *event, void *data, va_list args)
{
	char *attached_event = va_arg(args, char *);

	if(g_strcmp0(attached_event, "log") == 0) {
		if(count == 1) {
			$$(void, setLogHandler)(NULL); // restore log handler
			logInfo("Default log handler restored");
		}

		count--;
	}
}

/**
 * Event log handler redirecting all messages to the global log event
 *
 * @param module	the module in which the log message occured
 * @param type		the log type of the message
 * @param message	the log message
 */
static void eventLogHandler(const char *module, LogLevel level, const char *message)
{
	$(int, event, triggerEvent)(NULL, "log", module, level, message);
}
