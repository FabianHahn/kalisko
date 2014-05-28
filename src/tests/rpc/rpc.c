/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
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

#include "dll.h"
#include "glib.h"
#include "test.h"
#include "modules/rpc/rpc.h"
#include "modules/store/path.h"
#include "modules/store/store.h"
#include "modules/store/write.h"
#define API

MODULE_NAME("test_rpc");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("Test suite for the rpc module");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("rpc", 0, 0, 1));

#define RPC_SERVICE "/rpctest/testservice"

// String used to validate that the response is returned correctly.
#define RESULT_STRING_PATH "some_string"
#define RESULT_STRING_VALUE "some value"

// String used to validate the the request is passed correctly.
#define REQUEST_STRING_PATH "foo"
#define REQUEST_STRING_VALUE "some other value"

static int call_counter;
static bool argument_valid;

static Store *fakeService(Store *request)
{
  GString *serialized_store = writeStoreGString(request);
  logInfo("Handling fakeService request: \n%s", serialized_store->str);
  g_string_free(serialized_store, true);

	++call_counter;
	Store *value_store = getStorePath(request, REQUEST_STRING_PATH);
	if(value_store != NULL &&
	   g_strcmp0(REQUEST_STRING_VALUE, getStoreValueContent(value_store)) == 0) {
		argument_valid = true;
	}

	Store *response = createStore();
	setStorePath(response, RESULT_STRING_PATH, createStoreStringValue(RESULT_STRING_VALUE));
	return createStore();
}

static void setup()
{
	call_counter = 0;
	argument_valid = false;
	registerRpc(RPC_SERVICE,
	            NULL,  // Request schema.
	            NULL,  // Response schema.
	            &fakeService);
}

static void teardown()
{
	unregisterRpc(RPC_SERVICE);
}

TEST(calls_implementation)
{
	Store *request = createStore();
	Store *response;

	response = callRpc(RPC_SERVICE, request);
	TEST_ASSERT(response != NULL);
	TEST_ASSERT(call_counter == 1);
	freeStore(response);

	response = callRpc(RPC_SERVICE, request);
	TEST_ASSERT(response != NULL);
	TEST_ASSERT(call_counter == 2);
	freeStore(response);

	freeStore(request);
}

TEST(argument_passed)
{
	Store *request = createStore();
	setStorePath(request, REQUEST_STRING_PATH, createStoreStringValue(REQUEST_STRING_VALUE));
	Store *response = callRpc(RPC_SERVICE, request);
	TEST_ASSERT(argument_valid);
	freeStore(response);
	freeStore(request);
}

TEST(does_not_call_unknown)
{
	Store *request = createStore();
	Store *response = callRpc("/some/other/path", request);
	TEST_ASSERT(response == NULL);
	TEST_ASSERT(call_counter == 0);
}

TEST(unregistration)
{
	call_counter = 0;
	registerRpc(RPC_SERVICE,
	            NULL,  // Request schema.
	            NULL,  // Response schema.
	            &fakeService);
	unregisterRpc(RPC_SERVICE);

	Store *request = createStore();
	Store *response = callRpc(RPC_SERVICE, request);
	TEST_ASSERT(response == NULL);
	TEST_ASSERT(call_counter == 0);
}

TEST_SUITE_BEGIN(rpc)
	ADD_TEST_FIXTURE(RpcTest, &setup, &teardown);
	ADD_FIXTURED_TEST(calls_implementation, RpcTest);
	ADD_FIXTURED_TEST(argument_passed, RpcTest);
	ADD_FIXTURED_TEST(does_not_call_unknown, RpcTest);

	ADD_SIMPLE_TEST(unregistration);
TEST_SUITE_END
