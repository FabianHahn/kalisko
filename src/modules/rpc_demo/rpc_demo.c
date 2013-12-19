/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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
#include "modules/rpc/rpc.h"
#include "modules/store/path.h"
#include "modules/store/store.h"
#define API

// It is good practice to prepend a common path prefix used to identify the providing module.
#define RPC_GREETING_PATH "/rpcdemo/greeting"

MODULE_NAME("rpc_demo");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module demonstrates how to expose a simple rpc call using the rpc module.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("rpc", 0, 0, 1));

/*
Example usage of this module:

duh@laptop ~ $ telnet localhost 8889
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
rpc call /rpcdemo/greeting
other = 13
user = duh

"greeting" = "Hello, duh"
Connection closed by foreign host.
*/

static Store *greetingServiceImplementation(Store *request);

MODULE_INIT
{
	logWarning("Hello World");

	// TODO: Load request and response schema from greetings_request.store and greetings_response.store.
	registerRpc(RPC_GREETING_PATH,
	            NULL,  // Request schema.
	            NULL,  // Response schema.
	            &greetingServiceImplementation);

	return true;
}

MODULE_FINALIZE
{
}

// Simple implementation which extract a username from the request and returns a greeting.
Store *greetingServiceImplementation(Store *request)
{
	GString *greeting = g_string_new("Hello, ");
	Store *user_store = getStorePath(request, "user");
	if (user_store == NULL) {
		logWarning("No user provided");
		g_string_append(greeting, "unknown user");
	} else {
		g_string_append(greeting, (char *) getStoreValueContent(user_store));
	}

	Store *response = createStore();
	setStorePath(response, "greeting", createStoreStringValue(greeting->str));
	g_string_free(greeting, true);
	return response;
}
