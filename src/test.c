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
#include <stdio.h>
#include <stdlib.h>
#define API
#include "log.h"
#include "module.h"
#include "types.h"
#include "timer.h"
#include "util.h"
#include "test.h"
#include "memory_alloc.h"

static int test = 0;
static int passed = 0;
static int count = 0;

#define BUF 4096

int main(int argc, char **argv)
{
	g_thread_init(NULL);

	initMemory();
	initTimers();
	initLog();
	initModules();

	printf("Running test cases...\n");

	char *execpath = getExecutablePath();
	char *testdir = g_build_path("/", execpath, "tests", NULL);

	GError *error = NULL;
	GDir *tests = g_dir_open(testdir, 0, &error);

	if(tests == NULL) {
		fprintf(stderr, "Error: Could not open tests dir: %s\n", error != NULL ? error->message : "no error message");
		g_error_free(error);
		return EXIT_FAILURE;
	}

	char *node = NULL;

	while((node = (char *) g_dir_read_name(tests)) != NULL) {
		char *entry = g_build_path("/", testdir, node, NULL);

		if(g_file_test(entry, G_FILE_TEST_IS_DIR)) {
			char *modname = g_strjoin(NULL, "test_", node, NULL);

			if(!requestModule(modname)) {
				test = 0;
				reportTestResult(modname, "load", false, "Failed to load module");
			} else {
				revokeModule(modname);
			}

			free(modname);
		}

		free(entry);
	}

	g_dir_close(tests);

	free(testdir);
	free(execpath);

	freeModules();

	if(count) {
		double perc = 100.0 * passed / count;
		printf("\n%d of %d test cases passed (%.2f%%)\n", passed, count, perc);
	}

	return EXIT_SUCCESS;
}

API void incTestCount()
{
	test++;
}

API void resetTestCount()
{
	test = 0;
}

API void reportTestResult(char *testsuite, char *testcase, bool pass, char *error, ...)
{
	if(pass) {
		passed++;
		printf("[%s] %s: Passed\n", testsuite, testcase);
	} else {
		va_list va;
		char buffer[BUF];

		va_start(va, error);
		vsnprintf(buffer, BUF, error, va);

		printf("[%s] %s: Failed test #%d (%s)\n", testsuite, testcase, test, buffer);
	}

	fflush(stdout);

	count++;
}
