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
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 1, 1), MODULE_DEPENDENCY("store", 0, 5, 0));

TEST_CASE(xcall);
TEST_CASE(xcall_error);

TEST_SUITE_BEGIN(xcall)
	TEST_CASE_ADD(xcall);
	TEST_CASE_ADD(xcall_error);
TEST_SUITE_END

static GString *testXCallFunction(char *xcall)
{
	int fail = 0;
	Store *xcs = $(Store *, store, parseStoreString)(xcall);

	do { // dummy do-while to prevent mass if-then-else branching
		Store *function;
		if((function = $(Store *, store, getStorePath)(xcs, "xcall/function")) == NULL || function->type != STORE_STRING || g_strcmp0(function->content.string, "test") != 0) { // check if function name matches
			fail = 1;
			break;
		}

		Store *param;
		if((param = $(Store *, store, getStorePath)(xcs, "param")) == NULL || param->type != STORE_INTEGER || param->content.integer != 42) { // check if parameter correct
			fail = 2;
			break;
		}
	} while(false);

	$(void, store, freeStore)(xcs);

	GString *ret = g_string_new("");
	g_string_append_printf(ret, "fail = %d", fail);
	return ret;
}

TEST_CASE(xcall)
{
	TEST_ASSERT($(bool, xcall, addXCallFunction)("test", &testXCallFunction));
	GString *ret = $(GString *, xcall, invokeXCall)("param = 42; xcall = { function = test }");

	Store *rets = $(Store *, store, parseStoreString)(ret->str);

	g_string_free(ret, true);

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

	TEST_PASS;
}

TEST_CASE(xcall_error)
{
	GString *ret;
	Store *rets;
	Store *function;
	Store *error;

	ret = $(GString *, xcall, invokeXCall)("xcall = { function = does_not_exist }");
	rets = $(Store *, store, parseStoreString)(ret->str);
	g_string_free(ret, true);

	TEST_ASSERT((function = $(Store *, store, getStorePath)(rets, "xcall/function")) != NULL);
	TEST_ASSERT(function->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(function->content.string, "does_not_exist") == 0);

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	ret = $(GString *, xcall, invokeXCall)("error{{)({}error");
	rets = $(Store *, store, parseStoreString)(ret->str);
	g_string_free(ret, true);

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	ret = $(GString *, xcall, invokeXCall)("valid = { but = useless}");
	rets = $(Store *, store, parseStoreString)(ret->str);
	g_string_free(ret, true);

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(rets);

	TEST_PASS;
}
