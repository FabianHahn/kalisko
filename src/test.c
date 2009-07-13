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
#include "api.h"
#include "log.h"
#include "hooks.h"
#include "module.h"
#include "types.h"
#include "util.h"
#include "test.h"
#include "memory_alloc.h"

static int passed = 0;
static int count = 0;

#define BUF 4096

int main(int argc, char **argv)
{
	initMemory();
	initHooks();
	initLog();

	initModules();

	printf("Running test cases...\n");

	char *execpath = getExecutablePath();
	char *testdir = g_build_path("/", execpath, "tests", NULL);

	GError *testsDirError = NULL;
	GDir *tests = g_dir_open(testdir, 0, &testsDirError);

	if(tests == NULL) {
		fprintf(stderr, "Error: Could not open tests dir: %s\n", testsDirError != NULL ? testsDirError->message : "no error message");
		return EXIT_FAILURE;
	}

	const char *node = NULL;

	while((node = g_dir_read_name(tests)) != NULL) {
		char *entry = g_build_path("/", testdir, node, NULL);

		if(g_file_test(entry, G_FILE_TEST_IS_DIR)) {
			char *modname = g_strjoin(NULL, "test_", node, NULL);

			requestModule(modname);

			free(modname);
		}

		free(entry);
	}

	if(tests)
	{
		g_dir_close(tests);
	}

	if(testsDirError)
	{
		g_error_free(testsDirError);
	}

	free(testdir);
	free(execpath);

	freeModules();

	if(count) {
		double perc = 100.0 * passed / count;
		printf("\n%d of %d test cases passed (%.2f%%)\n", passed, count, perc);
	}

	freeHooks();

	return EXIT_SUCCESS;
}

/**
 * Reports a test result of a test case
 *
 * @param testsuite		the name of the test suite
 * @param testcase		the name of the test case
 * @param pass			did the test pass?
 * @param error			if it didn't pass, printf-like error message
 */
API void reportTestResult(char *testsuite, char *testcase, bool pass, char *error, ...)
{
	if(pass) {
		passed++;
		printf("[%s] %s: Pass\n", testsuite, testcase);
	} else {
		va_list va;
		char buffer[BUF];

		va_start(va, error);
		vsnprintf(buffer, BUF, error, va);

		printf("[%s] %s: Fail (%s)\n", testsuite, testcase, buffer);
	}

	count++;
}
