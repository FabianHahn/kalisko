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

#define API

MODULE_NAME("test_xcall_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the xcall_core module");
MODULE_VERSION(0, 1, 4);
MODULE_BCVERSION(0, 1, 4);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall_core", 0, 4, 0), MODULE_DEPENDENCY("store", 0, 6, 0));

TEST(log_hook);

static bool logSuccess;

TEST_SUITE_BEGIN(xcall_core)
	ADD_SIMPLE_TEST(log_hook);
TEST_SUITE_END

static Store *testXCallFunction(Store *xcall)
{
	Store *log_type;
	if((log_type = $(Store *, store, getStorePath)(xcall, "log_type")) == NULL || log_type->type != STORE_STRING) { // check if log_type exists
		logSuccess = false;
	}

	Store *message;
	if((message = $(Store *, store, getStorePath)(xcall, "message")) == NULL || message->type != STORE_STRING) { // check if message exists
		logSuccess = false;
	}

	return $(Store *, store, createStore)();
}

TEST(log_hook)
{
	logSuccess = true;

	TEST_ASSERT($(bool, xcall, addXCallFunction)("test", &testXCallFunction));
	Store *rets = $(Store *, xcall, invokeXCallByString)("listener = test; xcall = { function = attachLog }");

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) == NULL);

	Store *success;
	TEST_ASSERT((success = $(Store *, store, getStorePath)(rets, "success")) != NULL);
	TEST_ASSERT(success->type == STORE_INTEGER);
	TEST_ASSERT(success->content.integer == 1);

	$(void, store, freeStore)(rets);

	TEST_ASSERT(logSuccess);

	rets = $(Store *, xcall, invokeXCallByString)("listener = test; xcall = { function = detachLog }");

	TEST_ASSERT((error = $(Store *, store, getStorePath)(rets, "xcall/error")) == NULL);

	TEST_ASSERT((success = $(Store *, store, getStorePath)(rets, "success")) != NULL);
	TEST_ASSERT(success->type == STORE_INTEGER);
	TEST_ASSERT(success->content.integer == 1);

	$(void, store, freeStore)(rets);

	TEST_ASSERT($(bool, xcall, delXCallFunction)("test"));
}
