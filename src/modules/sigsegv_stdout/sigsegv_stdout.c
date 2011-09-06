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
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <stdio.h>

#include "dll.h"
#include "modules/event/event.h"
#include "modules/posix_signal/posix_signal.h"
#include "log.h"
#include "types.h"
#include "module.h"

#define API

MODULE_NAME("sigsegv_stdout");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Handles the SIGSEGV Ansi signal, writes the backtrace and exits gracefully");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 1, 1), MODULE_DEPENDENCY("posix_signal", 0, 0, 1));

#define SIGSEGV_BT_BUFFER_SIZE 100

static void handleSigsegv(void *subject, const char *event, void *custom_data, va_list args);

MODULE_INIT
{
	$(void, event, attachEventListener)(NULL, "posixSignal", NULL, handleSigsegv);
	handlePosixSignal(SIGSEGV);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "posixSignal", NULL, handleSigsegv);
}

static void handleSigsegv(void *subject, const char *event, void *custom_data, va_list args)
{
	void *buffer[SIGSEGV_BT_BUFFER_SIZE];
	int nptrs;

	nptrs = backtrace(buffer, SIGSEGV_BT_BUFFER_SIZE);

	printf("\n=== SIGSEGV BACKTRACE ===\n\n");
	backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);
	printf("\n=== SIGSEGV BACKTRACE END===\n");

	exit(EXIT_FAILURE);
}
