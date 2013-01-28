/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "dll.h"
#include "modules/event/event.h"
#include "log.h"
#include "types.h"
#include "module.h"

#define API

MODULE_NAME("posix_signal");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module connects POSIX signals to the event system");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 1, 1));

static void handleSignal(int sig, siginfo_t *info, void *context);

struct sigaction action;
struct sigaction defaultAction;
GList *enabledSignals;

MODULE_INIT
{
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = handleSignal;

	sigemptyset(&defaultAction.sa_mask);
	defaultAction.sa_handler = SIG_DFL;

	enabledSignals = NULL;

	return true;
}

MODULE_FINALIZE
{
	for(GList *iter = enabledSignals; iter != NULL; iter = iter->next) {
		sigaction(GPOINTER_TO_INT(iter->data), &defaultAction, NULL);
	}

	g_list_free(enabledSignals);
}

API void handlePosixSignal(int signal)
{
	if(g_list_find(enabledSignals, GINT_TO_POINTER(signal)) == NULL) {
		sigaction(signal, &action, NULL);
		enabledSignals = g_list_prepend(enabledSignals, GINT_TO_POINTER(signal));
		logInfo("Added POSIX signal to handle: %s", strsignal(signal));
	}
}

static void handleSignal(int sig, siginfo_t *info, void *context)
{
	logInfo("Caught signal: %s", strsignal(sig));

	$(void, event, triggerEvent)(NULL, "posixSignal", sig, info, context);
}
