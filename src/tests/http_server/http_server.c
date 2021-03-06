/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * Copyright (c) 2012, Google Inc.
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
#include "test.h"
#include "modules/http_server/http_server.h"
#define API

MODULE_NAME("test_http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("Test suite for the http_server module");
MODULE_VERSION(0, 0, 2);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("http_server", 0, 1, 3));

static HttpServer *server;
static HttpRequest *request;
static int counter;

static bool incrementCounter()
{
	++counter;
	return true;
}

static void setup()
{
	server = createHttpServer("12345");
	startHttpServer(server);
	registerHttpServerRequestHandler(server, "/path", &incrementCounter, NULL);
	registerHttpServerRequestHandler(server, "^/path2$", &incrementCounter, NULL);

	counter = 0;

	request = createHttpRequest();
	request->method = HTTP_REQUEST_METHOD_GET;
}

static void teardown()
{
	destroyHttpServer(server);
	destroyHttpRequest(request);
}

TEST(lifecycle)
{
	registerHttpServerRequestHandler(server, "/.*", &incrementCounter, NULL);
	unregisterHttpServerRequestHandler(server, "/.*", &incrementCounter, NULL);
}

TEST(handler)
{
	request->hierarchical = strdup("/path");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 1);
	destroyHttpResponse(response);
}

TEST(extra_symbols)
{
	request->hierarchical = strdup("/path2");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 1);
	destroyHttpResponse(response);
}

TEST(no_handler)
{
	request->hierarchical = strdup("/other_path");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 0);
	destroyHttpResponse(response);
}

TEST(partial_match)
{
	request->hierarchical = strdup("/something/path/something_else");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 0);
	destroyHttpResponse(response);
}

TEST(prefix_match)
{
	request->hierarchical = strdup("/path/something_else");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 0);
	destroyHttpResponse(response);
}

TEST(suffix_match)
{
	request->hierarchical = strdup("something_else/path");
	TEST_ASSERT(counter == 0);
	HttpResponse *response = handleHttpRequest(server, request);
	TEST_ASSERT(counter == 0);
	destroyHttpResponse(response);
}

TEST_SUITE_BEGIN(http_server)
	ADD_TEST_FIXTURE(HttpServerTest, &setup, &teardown);
	ADD_FIXTURED_TEST(lifecycle, HttpServerTest);
	ADD_FIXTURED_TEST(handler, HttpServerTest);
	ADD_FIXTURED_TEST(extra_symbols, HttpServerTest);
	ADD_FIXTURED_TEST(no_handler, HttpServerTest);
	ADD_FIXTURED_TEST(partial_match, HttpServerTest);
	ADD_FIXTURED_TEST(prefix_match, HttpServerTest);
	ADD_FIXTURED_TEST(suffix_match, HttpServerTest);
TEST_SUITE_END
