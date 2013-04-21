/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * Copyright (c) 2012, Google Inc.
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

#include <stdlib.h> // atoi
#include <glib.h>
#include <stdlib.h>
#include "dll.h"

#define API
#include "modules/http_server/http_server.h"
#include "modules/shared_http_server/shared_http_server.h"

#define BASE_URL "/" STR(KALISKO_MODULE)

#define MIRROR_URL "/mirror"
#define POST_DEMO_URL "/postdemo"
#define MATCH_EVERYTHING ".*"

MODULE_NAME("http_server_demo");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http service which demonstrates how to use the shared_http_server library.");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 3);
MODULE_DEPENDS(MODULE_DEPENDENCY("shared_http_server", 0, 0, 1));

static void appendTitle(HttpResponse *response);

static bool mirrorHandler(HttpRequest *request, HttpResponse *response, void *userdata);
static bool postDemoHandler(HttpRequest *request, HttpResponse *response, void *userdata);
static bool indexHandler(HttpRequest *request, HttpResponse *response, void *userdata);

static int post_demo_counter;

MODULE_INIT
{
	post_demo_counter = 0;
	registerSharedHttpServerRequestHandler(MIRROR_URL, &mirrorHandler, NULL);
	registerSharedHttpServerRequestHandler(POST_DEMO_URL, &postDemoHandler, NULL);
	registerSharedHttpServerRequestHandler(MATCH_EVERYTHING, &indexHandler, NULL);
	return true;
}

MODULE_FINALIZE
{
}

static void appendTitle(HttpResponse *response)
{
	appendHttpResponseContent(response, "<h1>Kalisko Webserver Demo</h1>");
}

/**
 * Prints a standard message, loops over all passed parameters and prints them.
 *
 * @param request			the HTTP request that should be handled
 * @param response			the HTTP response that will be sent back to the client
 * @param userdata			custom userdata
 * @result					true if successful
 */
static bool mirrorHandler(HttpRequest *request, HttpResponse *response, void *userdata)
{
	appendTitle(response);
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

/**
 * Demonstrates the POST support of the server.
 *
 * @param request			the HTTP request that should be handled
 * @param response			the HTTP response that will be sent back to the client
 * @param userdata			custom userdata
 * @result					true if successful
 */
static bool postDemoHandler(HttpRequest *request, HttpResponse *response, void *userdata)
{
	char *increment_param_key = "increment";

	if(request->method == HTTP_REQUEST_METHOD_POST) {
		char *increment = getHttpRequestParameter(request, increment_param_key);
		if(increment != NULL) {
			// If this fails, just increment by 0, which is ok.
			post_demo_counter += atoi(increment);
		}
	}

	appendTitle(response);
	appendHttpResponseContent(response, "The counter is at %d<br/><br/>", post_demo_counter);
	appendHttpResponseContent(response, "<form action=\"%s\" method=\"POST\">", BASE_URL POST_DEMO_URL);
	appendHttpResponseContent(response, "Increment by ");
	appendHttpResponseContent(response, "<input type=\"text\" name=\"increment\"><br>");
	appendHttpResponseContent(response, "<input type=\"submit\" value=\"Increment\"><br>");
	appendHttpResponseContent(response, "</form>");
	return true;
}

/**
 * Displays a default page and lists all supported functionality of this demo.
 *
 * @param request			the HTTP request that should be handled
 * @param response			the HTTP response that will be sent back to the client
 * @param userdata			custom userdata
 * @result					true if successful
 */
static bool indexHandler(HttpRequest *request, HttpResponse *response, void *userdata)
{
	appendTitle(response);
	appendHttpResponseContent(response, "<a href=%s>Mirror</a>", BASE_URL MIRROR_URL "?foo=bar");
	appendHttpResponseContent(response, "<br/>");
	appendHttpResponseContent(response, "<a href=%s>Post demo</a>", BASE_URL POST_DEMO_URL);
	return true;
}
