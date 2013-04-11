/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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

#include "test.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strdup

#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "module.h"
#include "timer.h"
#include "types.h"
#include "util.h"
#define API

#define TERMINAL_WIDTH 80

// TODO: Use the logging facilities instead of printf for these. Note that
// logging is disabled for tests (in SConstruct), so implementing selective
// logging is required for this. prtinf will do for now.
// The desired behavior would be to log everything form the modules of level
// warning or above and everything (unconditionally) from this file.
#define TEST_OUTPUT_INFO(...) printf(__VA_ARGS__); printf("\n");
#define TEST_OUTPUT_ERROR(...) fprintf(stderr, __VA_ARGS__); printf("\n");

/** Stores the number tests which passed */
static unsigned int tests_passed = 0;

/** Stores the number of tests which have been run so far */
static unsigned int tests_ran = 0;

/** Stores the test currently being run */
static TestCase *current_test_case = NULL;

/**
 * Stores a whitelist of regular expressions used to determine whether to
 * execute a test suite
 */
static GPtrArray *test_suite_whitelist = NULL;

static void populateWhitelist();
static bool isWhitelisted(char *suite_name);
static void appendRight(GString *message, char *suffix);
static TestFixture *createTestFixture(char *name, SetupFunction *setup, TeardownFunction *teardown);
static void destroyTestFixture(TestFixture *fixture);
static TestCase *createTestCase(char *name, TestFunction *function, TestFixture *fixture);
static void destroyTestCase(TestCase *test_case);
static bool testCaseFailed(TestCase *test_case);

int main(int argc, char **argv)
{
	g_thread_init(NULL);

	setArgc(argc);
	setArgv(argv);

	initMemory();
	initTimers();
	initLog(LOG_LEVEL_ERROR);
	initModules();

	test_suite_whitelist = g_ptr_array_new_with_free_func(&free);
	populateWhitelist();

	TEST_OUTPUT_INFO("Running test cases...\n");

	char *execpath = getExecutablePath();
	char *testdir = g_build_path("/", execpath, "tests", NULL);

	GError *error = NULL;
	GDir *tests = g_dir_open(testdir, 0, &error);

	if(tests == NULL) {
		TEST_OUTPUT_ERROR("Error: Could not open tests dir: %s\n", error != NULL ? error->message : "no error message");
		g_error_free(error);
		return EXIT_FAILURE;
	}

	char *node = NULL;

	while((node = (char *) g_dir_read_name(tests)) != NULL) {
		char *entry = g_build_path("/", testdir, node, NULL);

		if(g_file_test(entry, G_FILE_TEST_IS_DIR)) {
			char *modname = g_strjoin(NULL, "test_", node, NULL);
			if(!requestModule(modname)) {
				TEST_OUTPUT_ERROR("Error: Failed to load test module: %s", modname);
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

	if(tests_ran > 0) {
		double perc = 100.0 * tests_passed / tests_ran;
		TEST_OUTPUT_INFO("\n%d of %d test cases passed (%.2f%%)", tests_passed, tests_ran, perc);
	}

	g_ptr_array_free(test_suite_whitelist, true);

	return EXIT_SUCCESS;
}

API TestSuite *createTestSuite(char *name)
{
	TestSuite *result = ALLOCATE_OBJECT(TestSuite);
	result->name = strdup(name);
	result->test_cases = g_ptr_array_new_with_free_func((GDestroyNotify) &destroyTestCase);
	result->test_fixtures = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) &destroyTestFixture);
	return result;
}

API void destroyTestSuite(TestSuite *test_suite)
{
	free(test_suite->name);
	g_ptr_array_free(test_suite->test_cases, true);   // Frees(destroys) the contained structs
	g_hash_table_destroy(test_suite->test_fixtures);  // Frees(destroys) the contained structs
	free(test_suite);
}

API void addTestFixture(TestSuite *test_suite, char *name, SetupFunction *setup, TeardownFunction *teardown)
{
	assert(test_suite->test_fixtures != NULL);
	TestFixture *test_fixture = createTestFixture(name, setup, teardown);
	g_hash_table_replace(test_suite->test_fixtures, strdup(name), test_fixture);
}

API void addTest(TestSuite *test_suite, char *name, TestFunction *test_function, char *fixture_name)
{
	assert(test_suite->test_fixtures != NULL);
	assert(test_suite->test_cases != NULL);
	TestFixture *test_fixture = NULL;
	if(fixture_name != NULL) {
		test_fixture = g_hash_table_lookup(test_suite->test_fixtures, fixture_name);
	}
	TestCase *test_case = createTestCase(name, test_function, test_fixture);
	g_ptr_array_add(test_suite->test_cases, test_case);
}

API void runTestSuite(TestSuite *test_suite)
{
	if(test_suite_whitelist->len != 0 && !isWhitelisted(test_suite->name)) {
		return;
	}

	GPtrArray *cases = test_suite->test_cases;
	for(int i = 0; i < cases->len; ++i) {
		TestCase *test_case = g_ptr_array_index(cases, i);

		// Store the test case in the global pointer to allow redirecting the logging to the test case buffer
		current_test_case = test_case;

		GString *message = g_string_new("");
		g_string_append_printf(message, "Test case [%s] %s:", test_suite->name, test_case->name);

		TestFixture *fixture = test_case->test_fixture;
		if(fixture != NULL && fixture->setup_function != NULL) {
			fixture->setup_function();
		}
		test_case->test_function(test_case);
		if(fixture != NULL && fixture->teardown_function !=NULL) {
			fixture->teardown_function();
		}

		if(testCaseFailed(test_case)) {
			appendRight(message, "FAIL");
			g_string_append_printf(message, "\n   %s", test_case->error);
		} else {
			tests_passed++;
			appendRight(message, "PASS");
		}

		tests_ran++;
		current_test_case = NULL;

		TEST_OUTPUT_INFO("%s", message->str);
		g_string_free(message, true);
	}
}

API void failTest(TestCase *test_case, char* error, ...)
{
	free(test_case->error);
	va_list va;	
	va_start(va, error);
	test_case->error = g_strdup_vprintf(error, va);
}

static void populateWhitelist()
{
	requestModule("getopts");
	requestModule("string_util");

	typedef char* (*GetOptValueType)(char *opt, ...);
	GetOptValueType getOptValue = (GetOptValueType) getLibraryFunctionByName("getopts", "getOptValue");
	if(getOptValue == NULL) {
		TEST_OUTPUT_ERROR("Could not resolve function getOptValue, could not populate whitelist");
		return;
	}

	typedef size_t (*ParseCommaSeparatedType)(char *str, GPtrArray *out);
	ParseCommaSeparatedType parseCommaSeparated = (ParseCommaSeparatedType) getLibraryFunctionByName("string_util", "parseCommaSeparated");
	if(parseCommaSeparated == NULL) {
		TEST_OUTPUT_ERROR("Could not resolve parseCommaSeparated, could not populate whitelist");
		return;
	}

	char *module_list = getOptValue("test-modules", "t", NULL);  // Not owned.
	if(module_list != NULL) {
		size_t whitelisted = parseCommaSeparated(module_list, test_suite_whitelist);
		TEST_OUTPUT_INFO("Whitelisted %lu test suite names", whitelisted);
	}

	revokeModule("string_util");
	revokeModule("getopts");
}

static bool isWhitelisted(char *suite_name)
{
	for(int i = 0; i < test_suite_whitelist->len; ++i) {
		char *list_entry = g_ptr_array_index(test_suite_whitelist, i);
		if(strcmp(list_entry, suite_name) == 0) {
			return true;
		}
	}
	return false;
}

/**
 * Appends spaces and the provided suffix to message such that the suffix is
 * right-aligned in terminal output. If impossible, this just appends the suffix
 */
static void appendRight(GString *message, char *suffix)
{
	int free_space = TERMINAL_WIDTH - (strlen(suffix) + message->len);
	if (free_space < 0) {
		// No room on this line, just append (and it won't look pretty)
		g_string_append(message, suffix);
		return;
	}
	char *whitespace = g_strnfill(free_space, ' ');
	g_string_append_printf(message, "%s%s", whitespace, suffix);
	free(whitespace);
}

static TestFixture *createTestFixture(char *name, SetupFunction *setup, TeardownFunction *teardown)
{
	TestFixture *result = ALLOCATE_OBJECT(TestFixture);
	result->name = strdup(name);
	result->setup_function = setup;
	result->teardown_function = teardown;
	return result;
}

static void destroyTestFixture(TestFixture *test_fixture)
{
	if(test_fixture == NULL) {
		return;
	}
	free(test_fixture->name);
	free(test_fixture);
}

static TestCase *createTestCase(char *name, TestFunction *function, TestFixture *fixture)
{
	TestCase *result = ALLOCATE_OBJECT(TestCase);
	result->name = strdup(name);
	result->test_function = function;
	result->test_fixture = fixture;
	result->error = NULL;
	result->log_lines = g_ptr_array_new_with_free_func(&free);
	return result;
}

static void destroyTestCase(TestCase *test_case)
{
	if(test_case == NULL) {
		return;
	}
	free(test_case->name);
	free(test_case->error);
	g_ptr_array_free(test_case->log_lines, true);  // Frees the contained strings
	free(test_case);
}

static bool testCaseFailed(TestCase *test_case)
{
	return test_case->error != NULL;
}
