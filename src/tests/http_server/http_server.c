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

#include "dll.h"
#include "test.h"
#include "modules/http_server/http_server.h"
#define API

MODULE_NAME("test_http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("Test suite for the http_server module");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("http_server", 0, 1, 3));

static int counter;
static bool incrementCounter()
{
	++counter;
	return true;
}

TEST_CASE(lifecycle);

TEST_SUITE_BEGIN(http_server)
	TEST_CASE_ADD(lifecycle);
TEST_SUITE_END

TEST_CASE(lifecycle)
{
	HttpServer *server = createHttpServer("12345");
	startHttpServer(server);
	registerHttpServerRequestHandler(server, "/.*", &incrementCounter, NULL);
	unregisterHttpServerRequestHandler(server, "/.*", &incrementCounter, NULL);
	destroyHttpServer(server);
	TEST_PASS;
}
