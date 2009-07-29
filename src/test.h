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

#ifndef TEST_H
#define TEST_H

#include "types.h"

API void reportTestResult(char *testsuite, char *testcase, bool pass, char *error, ...);

/**
 * Initializes a test suite
 *
 * @param SUITE		the name of the test suite
 */
#define TEST_SUITE_BEGIN(SUITE) API void module_finalize() { } \
	API bool module_init() \
	{ \
		char *testsuite = #SUITE;

#define TEST_SUITE_END return true; \
	}

/**
 * Returns the internal function name of a test case
 *
 * @param CASE		the test case's name
 */
#define TEST_CASE_NAME(CASE) _test_case_ ## CASE

/**
 * Adds a test case to a test suite
 *
 * @param CASE		the test case's name
 */
#define TEST_CASE_ADD(CASE) TEST_CASE_NAME(CASE)(testsuite, #CASE)

/**
 * Defines a test case
 *
 * @param CASE		the test case's name
 */
#define TEST_CASE(CASE) static void TEST_CASE_NAME(CASE)(char *testsuite, char *testcase)

#ifdef DLL_API_IMPORT
/**
 * Checks if an expression holds, and fails the test case if it doesn't
 *
 * @param EXPR		an expression to check
 */
#define TEST_ASSERT(EXPR) if(!(EXPR)) { TEST_FAIL("Assertion failed: " #EXPR); }

/**
 * Passes a test case
 */
#define TEST_PASS $$(void, reportTestResult)(testsuite, testcase, true, NULL); return

/**
 * Fails a test case with an error message
 *
 * @param ERROR		printf-like error message
 */
#define TEST_FAIL(ERROR, ...) $$(void, reportTestResult)(testsuite, testcase, false, ERROR, ##__VA_ARGS__); return
#endif

#endif
