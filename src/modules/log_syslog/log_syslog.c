/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "modules/event/event.h"
/*
 * Now we do some nasty stuff because of name conflicts between log.h and syslog.h.
 * First we undef some macros which are defined by log.h in dll.h. After that we
 * include syslog.h and load log.h after api.h as we only need the enum and not the
 * logWarning(...), logNotice(...), ... macros of log.h.
 */
#undef LOG_WARNING
#undef LOG_INFO
#undef LOG_DEBUG

#include <syslog.h>

#define API
#include "log.h"

MODULE_NAME("log_syslog");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Kalisko log provider for syslog on POSIX.1-2001 systems.");
MODULE_VERSION(0, 0, 2);
MODULE_BCVERSION(0, 0, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 1, 2), MODULE_DEPENDENCY("log_event", 0, 1, 1));

static void listener_log(void *subject, const char *event, void *data, va_list args);


MODULE_INIT
{
	openlog("kalisko", LOG_CONS, LOG_USER);

	$(void, event, attachEventListener)(NULL, "log", NULL, &listener_log);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "log", NULL, &listener_log);

	closelog();
}

/**
 * Log message listener to delegate the log message to syslogd.
 */
static void listener_log(void *subject, const char *event, void *data, va_list args)
{
	const char *module = va_arg(args, const char *);
	LogLevel level = va_arg(args, LogLevel);
	char *message = va_arg(args, char *);

	switch(level) {
		case LOG_LEVEL_ERROR:
			syslog(LOG_ERR, "[%s] %s", module, message);
		break;
		case LOG_LEVEL_WARNING:
			syslog(LOG_WARNING, "[%s] %s", module, message);
		break;
		case LOG_LEVEL_NOTICE:
			syslog(LOG_NOTICE, "[%s] %s", module, message);
		break;
		case LOG_LEVEL_INFO:
			syslog(LOG_INFO, "[%s] %s", module, message);
		break;
		default:
		break;
	}
}
