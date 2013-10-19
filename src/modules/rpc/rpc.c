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

#include <stdarg.h>
#include <glib.h>
#include "dll.h"
#include "modules/event/event.h"
#include "modules/rpc/rpc.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#define API

MODULE_NAME("rpc");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides an easy way to implement an RPC interface built on top of stores.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(
		MODULE_DEPENDENCY("event", 0, 1, 2),
		MODULE_DEPENDENCY("socket", 0, 7, 0),
		MODULE_DEPENDENCY("store", 0, 5, 3));

MODULE_INIT
{
	logInfo("Hello World from the RPC module");
	return true;
}

MODULE_FINALIZE
{

}

API RpcInterface *createRpcInterface(Store *request_schema, Store *response_schema)
{
	RpcInterface *result = ALLOCATE_OBJECT(RpcInterface);
	result->request_schema = request_schema;
	result->response_schema = response_schema;
	return result;
}

API void destroyRpcInterface(RpcInterface *interface)
{
	free(interface);
}
