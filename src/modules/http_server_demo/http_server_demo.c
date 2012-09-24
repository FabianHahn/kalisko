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
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("http_server", 0, 0, 1));

static HttpServer *server;

/** Prints a standard message, loops over all passed parameters and prints them. */
static bool demoHandler(HttpRequest *request, HttpResponse *response)
{
	// TODO: Place this functionality in the library
	GString *content = g_string_new("Kalisko now has a web server! Oh yes, and hello world!<br/><br/>");
	
	GHashTable *params = getParameters(request);
	if(g_hash_table_size(params) > 0) {
		g_string_append(content, "Parameters:<br/>");

		GHashTableIter iter;
		gpointer vkey, vvalue;
		g_hash_table_iter_init (&iter, params);
		while (g_hash_table_iter_next (&iter, &vkey, &vvalue)) {
			char *key = vkey, *value = vvalue;
			g_string_append_printf(content, "Key: %s, Value: %s<br/>", key, value);
		} 
	}

	response->content = g_string_free(content, false);
	return true;
}

MODULE_INIT
{
	server = createHttpServer(PORT);
	registerRequestHandler(server, MATCH_EVERYTHING, &demoHandler);
	if (!startHttpServer(server)) {
		LOG_ERROR("Failed to start HTTP server");
		return false;
	}
	return true;
}

MODULE_FINALIZE
{
	destroyHttpServer(server);
}
