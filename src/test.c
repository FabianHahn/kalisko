/**
 * @file test.c
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

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "api.h"
#include "log.h"
#include "hooks.h"
#include "module.h"
#include "types.h"
#include "test.h"

static int passed = 0;
static int count = 0;

#define BUF 4096

int main(int argc, char **argv)
{
	initHooks();
	initLog();

	initModules();

	printf("Running test cases...\n");

	DIR *tests;

	if((tests = opendir("tests")) == NULL) {
		fprintf(stderr, "Error: Could not open tests dir.");
		return EXIT_FAILURE;
	}

	struct dirent *node;

	while((node = readdir(tests)) != NULL) {
		if(strcmp(node->d_name, ".") == 0 || strcmp(node->d_name, "..") == 0) {
			continue;
		}

		GString *entry = g_string_new("tests/");
		g_string_append(entry, node->d_name);

		struct stat properties;

		if(stat(entry->str, &properties) == -1) {
			fprintf(stderr, "Error: Could not stat %s", entry->str);
			return EXIT_FAILURE;
		}

		g_string_free(entry, TRUE);

		if(properties.st_mode & S_IFDIR) {
			GString *modname = g_string_new("test_");
			g_string_append(modname, node->d_name);

			requestModule(modname->str);

			g_string_free(modname, TRUE);
		}
	}

	freeModules();

	if(count) {
		double perc = 100.0 * passed / count;
		printf("\n%d of %d test cases passed (%.2f%%)\n", passed, count, perc);
	}

	freeHooks();

	return EXIT_SUCCESS;
}

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
