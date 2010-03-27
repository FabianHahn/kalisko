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

#include "dll.h"
#include "test.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"

#include "api.h"

MODULE_NAME("test_xcall");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the xcall module");
MODULE_VERSION(0, 1, 6);
MODULE_BCVERSION(0, 1, 6);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 2, 0), MODULE_DEPENDENCY("store", 0, 5, 3));

TEST_CASE(xcall);
TEST_CASE(xcall_error);

TEST_SUITE_BEGIN(xcall)
	TEST_CASE_ADD(xcall);
	TEST_CASE_ADD(xcall_error);
TEST_SUITE_END

static Store *testXCallFunction(Store *xcall)
{
	int fail = 0;

	do { // dummy do-while to prevent mass if-then-else branching
		Store *function;
		if((function = $(Store *, store, getStorePath)(xcall, "xcall/function")) == NULL || function->type != STORE_STRING || g_strcmp0(function->content.string, "test") != 0) { // check if function name matches
			fail = 1;
			break;
		}

		Store *param;
		if((param = $(Store *, store, getStorePath)(xcall, "param")) == NULL || param->type != STORE_INTEGER || param->content.integer != 42) { // check if parameter correct
			fail = 2;
			break;
		}
	} while(false);

	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "fail", $(Store *, store, createStoreIntegerValue)(fail));
	return ret;
}

TEST_CASE(xcall)
{
	TEST_ASSERT($(bool, xcall, addXCallFunction)("test", &testXCallFunction));
	Store *rets = $(Store *, xcall, invokeXCallByString)("param = 42; xcall = { function = test }");

	Store *function;
	TEST_ASSERT((function = $(Store *, store, getStorePath)(rets, "xcall/function")) != NULL);
	TEST_ASSERT(function->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(function->content.string, "test") == 0);

	Store *params;
	TEST_ASSERT((params = $(Store *, store, getStorePath)(rets, "xcall/params")) != NULL);
	TEST_ASSERT(params->type == STORE_ARRAY);

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_INTEGER);

	Store *fail;
	TEST_ASSERT((fail = $(Store *, store, getStorePath)(rets, "fail")) != NULL);
	TEST_ASSERT(fail->type == STORE_INTEGER);
	TEST_ASSERT(fail->content.integer == 0);

	$(void, store, freeStore)(rets);
	$(void, xcall, delXCallFunction)("test");

	TEST_PASS;
}

TEST_CASE(xcall_error)
{
	Store *rets;
	Store *function;
	Store *error;

	rets = $(Store *, xcall, invokeXCallByString)("xcall = { function = does_not_exist }");

	TEST_ASSERT((function = $(Store *, store, getStorePath)(rets, "xcall/function")) != NULL);
	TEST_ASSERT(function->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(function->content.string, "does_not_exist") == 0);

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	rets = $(Store *, xcall, invokeXCallByString)("error{{)({}error");

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	rets = $(Store *, xcall, invokeXCallByString)("valid = { but = useless}");

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	TEST_PASS;
}
