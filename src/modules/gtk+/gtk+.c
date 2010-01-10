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

#include <gtk/gtk.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "timer.h"
#include "module.h"
#include "util.h"

#include "api.h"
#include "modules/gtk+/gtk+.h"

MODULE_NAME("gtk+");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Basic module for GTK+ bases Kalisko modules.");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 2);
MODULE_NODEPS;

#ifndef GTK_MAIN_TIMEOUT
#define GTK_MAIN_TIMEOUT 5000
#endif

TIMER_CALLBACK(GTK_MAIN_LOOP);

static bool isLoopRunning;
static GTimeVal *lastScheduledPollTime;

MODULE_INIT
{
	char **argv = getArgv();
	int argc = getArgc();

	gtk_init(&argc, &argv);

	setArgv(argv);
	setArgc(argc);

	isLoopRunning = false;
	return true;
}

MODULE_FINALIZE
{
	stopGtkLoop();
}

TIMER_CALLBACK(GTK_MAIN_LOOP)
{
	gtk_main_iteration_do(FALSE);
	lastScheduledPollTime = TIMER_ADD_TIMEOUT(GTK_MAIN_TIMEOUT, GTK_MAIN_LOOP);
}

API void runGtkLoop()
{
	if(!isLoopRunning) {
		isLoopRunning = true;
		lastScheduledPollTime = TIMER_ADD_TIMEOUT(GTK_MAIN_TIMEOUT, GTK_MAIN_LOOP);
	}
}

API void stopGtkLoop()
{
	if(isLoopRunning) {
		TIMER_DEL(lastScheduledPollTime);
		isLoopRunning = false;
	}
}

API bool isGtkLoopRunning()
{
	return isLoopRunning;
}
