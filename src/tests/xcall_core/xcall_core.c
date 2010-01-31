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

MODULE_NAME("test_xcall_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the xcall_core module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall_core", 0, 1, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

TEST_CASE(log_hook);

static bool logSuccess;

TEST_SUITE_BEGIN(xcall_core)
	TEST_CASE_ADD(log_hook);
TEST_SUITE_END

static GString *testXCallFunction(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);

	Store *log_type;
	if((log_type = $(Store *, store, getStorePath)(xcs, "log_type")) == NULL || log_type->type != STORE_STRING) { // check if log_type exists
		logSuccess = false;
	}

	Store *message;
	if((message = $(Store *, store, getStorePath)(xcs, "message")) == NULL || message->type != STORE_STRING) { // check if message exists
		logSuccess = false;
	}

	$(void, store, freeStore)(xcs);

	return g_string_new("");
}

TEST_CASE(log_hook)
{
	logSuccess = true;

	TEST_ASSERT($(bool, xcall, addXCallFunction)("test", &testXCallFunction));
	GString *ret = $(GString *, xcall, invokeXCall)("listener = test; xcall = { function = attachLog }");

	Store *rets = $(Store *, store, parseStoreString)(ret->str);

	g_string_free(ret, true);

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_INTEGER);

	Store *success;
	TEST_ASSERT((success = $(Store *, store, getStorePath)(rets, "success")) != NULL);
	TEST_ASSERT(success->type == STORE_INTEGER);
	TEST_ASSERT(success->content.integer == 1);

	$(void, store, freeStore)(rets);

	TEST_ASSERT(logSuccess);

	ret = $(GString *, xcall, invokeXCall)("listener = test; xcall = { function = detachLog }");

	rets = $(Store *, store, parseStoreString)(ret->str);

	g_string_free(ret, true);

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_INTEGER);

	TEST_ASSERT((success = $(Store *, store, getStorePath)(rets, "success")) != NULL);
	TEST_ASSERT(success->type == STORE_INTEGER);
	TEST_ASSERT(success->content.integer == 1);

	$(void, store, freeStore)(rets);

	TEST_ASSERT($(bool, xcall, delXCallFunction)("test"));

	TEST_PASS;
}
