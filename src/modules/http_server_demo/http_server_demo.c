/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *		 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *		 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *			 in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <glib.h>
#include <stdlib.h>
#include "dll.h"

#define API
#include "modules/http_server/http_server.h"

#define PORT "1337"
#define MATCH_EVERYTHING "^/test.*"

MODULE_NAME("http_server_demo");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server which demonstrates how to use the http server library.");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("http_server", 0, 1, 1));

static bool demoHandler(HttpRequest *request, HttpResponse *response);

static HttpServer *server;

MODULE_INIT
{
	server = createHttpServer(PORT);
	registerHttpServerRequestHandler(server, MATCH_EVERYTHING, &demoHandler);
	if(!startHttpServer(server)) {
		LOG_ERROR("Failed to start HTTP server");
		return false;
	}
	return true;
}

MODULE_FINALIZE
{
	destroyHttpServer(server);
}

/**
 * Prints a standard message, loops over all passed parameters and prints them.
 *
 * @param request			the HTTP request that should be handled
 * @param response			the HTTP response that will be sent back to the client
 * @result					true if successful
 */
static bool demoHandler(HttpRequest *request, HttpResponse *response)
{
	appendHttpResponseContent(response, "Kalisko now has a web server! Oh yes, and hello world!<br/><br/>");

	if(g_hash_table_size(request->parameters) > 0) {
		appendHttpResponseContent(response, "Parameters:<br/>");

		GHashTableIter iter;
		char *key;
		char *value;
		g_hash_table_iter_init(&iter, request->parameters);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			appendHttpResponseContent(response, "Key: %s, Value: %s<br/>", key, value);
		}
	}

	return true;
}
