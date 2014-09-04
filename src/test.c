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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strdup

#include "dll.h"
#define API
#include "log.h"
#include "memory_alloc.h"
#include "module.h"
#include "timer.h"
#include "types.h"
#include "util.h"
#include "test.h"

#define TERMINAL_WIDTH 80

/** Stores the number tests which passed */
static unsigned int tests_passed = 0;

/** Stores the number of tests which have been run so far */
static unsigned int tests_ran = 0;

/** Stores the test currently being run */
static TestCase *current_test_case = NULL;

/** Stores the messages logged during initialization of the modules under test */
// TODO: Implement a command-line flag for displaying these if anything fails.
static GPtrArray *module_init_log_lines;

/**
 * Stores a whitelist of regular expressions used to determine whether to
 * execute a test suite
 */
static GPtrArray *test_suite_whitelist = NULL;
static void populateWhitelist();
static bool isWhitelisted(char *suite_name);

static TestFixture *createTestFixture(char *name, SetupFunction *setup, TeardownFunction *teardown);
static void destroyTestFixture(TestFixture *fixture);
static TestCase *createTestCase(char *name, TestFunction *function, TestFixture *fixture);
static void destroyTestCase(TestCase *test_case);
static bool testCaseFailed(TestCase *test_case);
static void appendRight(GString *message, char *suffix);

/** A custom log handler used to decide when messages get logged. */
static LogHandler *current_log_handler;
static void listener_log(void *subject, const char *event, void *data, va_list args);
static void setupLogHandling();
static void teardownLogHandling();
static void testCaseLogHandler(const char *name, LogLevel level, const char *message);
static void testInitLogHandler(const char *name, LogLevel level, const char *message);
static void stderrLogHandler(const char *name, LogLevel level, const char *message);

int main(int argc, char **argv)
{
	setArgc(argc);
	setArgv(argv);

	initMemory();
	initTimers();
	initLog(LOG_LEVEL_NOTICE_UP);
	initModules();

	test_suite_whitelist = g_ptr_array_new_with_free_func(&free);
	module_init_log_lines = g_ptr_array_new_with_free_func(&free);
	populateWhitelist();
	setupLogHandling();

	logNotice("Running test cases...");
	char *execpath = getExecutablePath();
	char *testdir = g_build_path("/", execpath, "tests", NULL);

	GError *error = NULL;
	GDir *tests = g_dir_open(testdir, 0, &error);

	if(tests == NULL) {
		logError("Could not open tests dir: %s\n", error != NULL ? error->message : "no error message");
		g_error_free(error);
		return EXIT_FAILURE;
	}

	char *node = NULL;
	while((node = (char *) g_dir_read_name(tests)) != NULL) {
		char *entry = g_build_path("/", testdir, node, NULL);
		if(g_file_test(entry, G_FILE_TEST_IS_DIR)) {
			char *modname = g_strjoin(NULL, "test_", node, NULL);
			current_log_handler = &testInitLogHandler;
			if(!requestModule(modname)) {
				current_log_handler = &stderrLogHandler;
				logError("Failed to load test module: %s", modname);

				GString *failMessage = g_string_new("");
				g_string_append_printf(failMessage, "Logs recorded during failed loading of '%s':", modname);
				for(int j = 0; j < module_init_log_lines->len; ++j) {
					char *line = g_ptr_array_index(module_init_log_lines, j);
					g_string_append_printf(failMessage, "\n  %s", line);
				}

				logNotice("%s", failMessage->str);

				g_ptr_array_set_size(module_init_log_lines, 0);
				g_string_free(failMessage, true);
			} else {
				revokeModule(modname);
				current_log_handler = &stderrLogHandler;
			}
			free(modname);
		}
		free(entry);
	}

	if(tests_ran > 0) {
		double perc = 100.0 * tests_passed / tests_ran;
		logNotice("%d of %d test cases passed (%.2f%%)", tests_passed, tests_ran, perc);
	}

	teardownLogHandling();

	g_dir_close(tests);
	free(testdir);
	free(execpath);
	freeModules();

	g_ptr_array_free(test_suite_whitelist, true);
	g_ptr_array_free(module_init_log_lines, true);
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
		current_log_handler = &testCaseLogHandler;

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

		tests_ran++;
		current_log_handler = &stderrLogHandler;
		current_test_case = NULL;

		if(testCaseFailed(test_case)) {
			appendRight(message, "FAIL");
			g_string_append_printf(message, "\n    %s\n", test_case->error);

			// Test failed, so dump the logs recorded during the test
			// TODO: It might be more helpful to just record that the test
			// failed and dump the failing tests and the logs at the very end.
			g_string_append(message, "  Logs recorded during failed test:\n");
			for(int j = 0; j < test_case->log_lines->len; ++j) {
				char *line = g_ptr_array_index(test_case->log_lines, j);
				g_string_append_printf(message, "    %s\n", line);
			}
			logNotice("%s", message->str);
		} else {
			tests_passed++;
			appendRight(message, "PASS");
			logNotice("%s", message->str);
		}

		g_string_free(message, true);
		current_log_handler = &testInitLogHandler;
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
	// TODO: Add the modules actually used here as dependencies in the
	// SConscript file in order to make sure they get built.
	requestModule("getopts");
	requestModule("string_util");

	typedef char* (*GetOptValueType)(char *opt, ...);
	GetOptValueType getOptValue = (GetOptValueType) getLibraryFunctionByName("getopts", "getOptValue");
	if(getOptValue == NULL) {
		logError("Could not resolve function getOptValue, could not populate whitelist");
		return;
	}

	typedef size_t (*ParseCommaSeparatedType)(char *str, GPtrArray *out);
	ParseCommaSeparatedType parseCommaSeparated = (ParseCommaSeparatedType) getLibraryFunctionByName("string_util", "parseCommaSeparated");
	if(parseCommaSeparated == NULL) {
		logError("Could not resolve parseCommaSeparated, could not populate whitelist");
		return;
	}

	char *module_list = getOptValue("test-modules", "t", NULL);  // Not owned.
	if(module_list != NULL) {
		size_t whitelisted = parseCommaSeparated(module_list, test_suite_whitelist);
		logNotice("Whitelisted %lu test suite names", whitelisted);
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


static void listener_log(void *subject, const char *event, void *data, va_list args)
{
	const char *module = va_arg(args, const char *);
	LogLevel level = va_arg(args, LogLevel);
	char *message = va_arg(args, char *);
	current_log_handler(module, level, message);
}

static void setupLogHandling()
{
	// TODO: Add the modules actually used here as dependencies in the
	// SConscript file in order to make sure they get built.
	requestModule("log_event");
	typedef void (*AttachEventListenerType) (void *, char *, void *, void *);
	AttachEventListenerType attachEventListener = (AttachEventListenerType) getLibraryFunctionByName("event", "attachEventListener");

	current_log_handler = &stderrLogHandler;
	attachEventListener(NULL, "log", NULL, &listener_log);
}

static void teardownLogHandling()
{
	typedef void (*DetachEventListenerType) (void *, char *, void *, void *);
	DetachEventListenerType detachEventListener = (DetachEventListenerType) getLibraryFunctionByName("event", "detachEventListener");

	detachEventListener(NULL, "log", NULL, &listener_log);
	current_log_handler = NULL;
	revokeModule("log_event");
}

/**
 * Stores the logged messages in the buffer of the current test case.
 *
 * @param name       	the module from which the message is coming
 * @param level         the log level of the message to be logged
 * @param message       the original logging message
 */
static void testCaseLogHandler(const char *name, LogLevel level, const char *message)
{
	// We ignore the log level (and always log) since these logs are only shown on test failure.
	char *formatted = formatLogMessage(name, level, message);
	g_ptr_array_add(current_test_case->log_lines, formatted);
}

/**
 * Puts the logged messages in a global buffer used to track how module initialization went.
 *
 * @param name       	the module from which the message is coming
 * @param level         the log level of the message to be logged
 * @param message       the original logging message
 */
static void testInitLogHandler(const char *name, LogLevel level, const char *message)
{
	if(shouldLog(level)) {
		char *formatted = formatLogMessage(name, level, message);
		g_ptr_array_add(module_init_log_lines, formatted);
	}
}

/**
 * Formats a log message and dumps it to stderr
 *
 * @param name       	the module from which the message is coming
 * @param level         the log level of the message to be logged
 * @param message       the original logging message
 */
static void stderrLogHandler(const char *name, LogLevel level, const char *message)
{
	if(shouldLog(level)) {
		char *formatted = formatLogMessage(name, level, message);
		fprintf(stderr, "%s\n", formatted);
		free(formatted);
	}
	fflush(stderr);
}
