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

/**
 * @mainpage Kalisko API Documentation
 *
 * Go to <a href="http://dev.kalisko.org" target="_top"><b>Trac</b></a> for more documentation,
 * suggestions, bugs and much more.
 *
 * And do not forget to check out our <a href="http://api.kalisko.org/depgraphs/" target="_top"><b>Module Dependency Graphs</b></a>
 */

#ifndef MIN_SLEEP_TIME
#define MIN_SLEEP_TIME 1000
#endif

#include <stdlib.h>
#include "api.h"
#include "log.h"
#include "timer.h"
#include "module.h"
#include "memory_alloc.h"
#include "util.h"

int main(int argc, char **argv)
{
	g_thread_init(NULL);

	setArgc(argc);
	setArgv(argv);

	initMemory();
	initTimers();
	initLog();
	initModules();

	logMessage("core", LOG_TYPE_INFO, "Core startup complete - welcome to the Kalisko framework!");

	requestModule("module_perform");

	logMessage("core", LOG_TYPE_DEBUG, "Entering Kalisko event loop");
	while(hasMoreTimerCallbacks()) {
		int sleepTime = getCurrentSleepTime();
		g_usleep(sleepTime < MIN_SLEEP_TIME ? MIN_SLEEP_TIME : sleepTime);
		notifyTimerCallbacks();
	}
	logMessage("core", LOG_TYPE_DEBUG, "Leaving Kalisko event loop");

	freeModules();
	freeTimers();

	logMessage("core", LOG_TYPE_INFO, "Kalisko core shutting down - goodbye!");

	return EXIT_SUCCESS;
}
