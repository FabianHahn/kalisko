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

#ifndef TEST_H
#define TEST_H

#include "module.h"
#include "types.h"

// Forward declaration used for function types
struct TestCaseStruct;

/**
 * A function which can be used as setup in a test fixture
 */
typedef void (SetupFunction)();

/**
 * A function which can be used as teardown in a test fixture
 */
typedef void (TeardownFunction)();

/**
 * A function which can be used as a test case
 */
typedef void (TestFunction)(struct TestCaseStruct*);

/**
 * Represents an environment for unit tests in which setup is called
 * before each test and teardown is called after each test
 */
typedef struct
{
	/** Identifies this fixture */
	char *name;
	/** Optional setup function */
	SetupFunction *setup_function;
	/** Optional teardown function */
	TeardownFunction *teardown_function;
} TestFixture;

/**
 * Represents a basic unit test which can pass or fail
 */
typedef struct TestCaseStruct
{
	/** Identifies this test */
	char *name;
	/** Stores the function which is run as a test */
	TestFunction *test_function;
	/** Optional fixture for setup and teardown */
	TestFixture *test_fixture;
	/** Optional error describing what went wrong */
	char *error;
	/** Array of raw strings storing log lines during test execution */
	GPtrArray *log_lines;
} TestCase;

/**
 * Represents a collection of unit tests. Groups them to be
 * run in sequence
 */
typedef struct
{
	/** Identifies this test suite */
	char *name;
	/** Stores the test cases for in-order execution */
	GPtrArray *test_cases;
	/** Stores all used fixtures by name */
	GHashTable *test_fixtures;
} TestSuite;

/**
 * Constructor for the TestSuite struct. The caller is responsible for
 * eventually calling destroyTestSuite
 *
 * @param name			the name of the new test suite, will be used
 *						to identify the test suite in test output
 * @result				the new TestSuite struct
 */
API TestSuite *createTestSuite(char *name);

/**
 * Frees all allocated memory associated with a TestSuite
 *
 * @param test_suite 	the TestSuite to be destroyed
 */
API void destroyTestSuite(TestSuite *test_suite);

/**
 * Makes a test fixture available in a test suite. After this call, tests
 * can make use of the new fixture
 *
 * @param test_suite 	the TestSuite to which a fixture is added
 * @param name 			the name of the fixture
 * @param setup			a setup function for the fixture
 * @param teardown		a teardown function for the fixture
 */
API void addTestFixture(TestSuite *test_suite, char *name, SetupFunction *setup, TeardownFunction *teardown);

/**
 * Adds a new test to a test suite. If no fixture with the provided name
 * exists (or the fixture name if NULL), adds a test without a fixture
 *
 * @param test_suite	the test suite to add the test to
 * @param name			the name of the test
 * @param function		a test function to be executed when running the test
 * @param fixture_name	the name of a fixture registered with this test suite
 */
API void addTest(TestSuite *test_suite, char *name, TestFunction *function, char *fixture_name);

/**
 * Executes all tests in the test suite.
 */
API void runTestSuite(TestSuite *test_suite);

/**
 * Fails the given test case and stores the error message
 *
 * @param error			printf-style reason for the error
 */
API void failTest(TestCase *test_case, char *error, ...);

/**
 * Defines a new test case
 *
 * @param CASE			the test case's name
 */
#define TEST(TEST_NAME) static void TEST_NAME(TestCase *test_case)

/**
 * Adds a new fixture to the current test module
 *
 * @param FIXTURE_NAME	name identifying the fixture
 * @param SETUP			optional setup function called before each test
 * @param TEARDOWN		optional teardown function called after each test
 */
#define ADD_TEST_FIXTURE(FIXTURE_NAME, SETUP, TEARDOWN) addTestFixture(test_suite, #FIXTURE_NAME, SETUP, TEARDOWN);

/**
 * Adds a test case without fixture to the current test suite
 *
 * @param TEST_NAME		the name of the test to add
 */
#define ADD_SIMPLE_TEST(TEST_NAME) addTest(test_suite, #TEST_NAME, TEST_NAME, NULL);

/**
 * Adds a test case with a fixture to the current test suite.
 *
 * @param TEST_NAME		the name of the test to add
 * @param FIXTURE_NAME	the name of the fixture to use for this test
 */
#define ADD_FIXTURED_TEST(TEST_NAME, FIXTURE_NAME) addTest(test_suite, #TEST_NAME, TEST_NAME, #FIXTURE_NAME);

/**
 * Initializes a test suite
 *
 * @param SUITE		the name of the test suite
 */
#define TEST_SUITE_BEGIN(SUITE_NAME) MODULE_INIT \
	{ \
		TestSuite *test_suite = createTestSuite(#SUITE_NAME);

/**
 * Cleans up the test suite
 */
#define TEST_SUITE_END runTestSuite(test_suite); destroyTestSuite(test_suite); return true; \
	}

/**
 * Checks if an expression holds, and fails the test case if it doesn't
 *
 * @param EXPR		an expression to check
 */
#define TEST_ASSERT(EXPR) if(!(EXPR)) { TEST_FAIL("%s:%d: Assertion failed: " #EXPR, __FILE__, __LINE__); }

/**
 * Fails a test case with an error message
 *
 * @param ERROR		printf-like error message
 */
#define TEST_FAIL(ERROR, ...) failTest(test_case, ERROR, ##__VA_ARGS__); return;

#endif
