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
#include "modules/store/clone.h"
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

/**
 * Struct representing an RPC implementation.
 */
typedef struct
{
	/** The path under which the RPC is available. */
	char *path;
  /** Stores (pun intended) the request schema of the service. */
  Store *request_schema;
  /** Stores the response schema of the service. */
  Store *response_schema;
	/** The function to be called for this RPC. */
	RpcImplementation *implementation;
} RpcService;

/** Stores the mapping from path to the correspinding services. **/
static GHashTable *service_map;

static RpcService *createRpcService(char *path, Store *requestSchema, Store *responseSchema, RpcImplementation *implementation);
static void destroyRpcService(RpcService *rpcService);

MODULE_INIT
{
	service_map = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) &destroyRpcService);
	logInfo("Hello World from the rpc module");
	return true;
}

MODULE_FINALIZE
{

}

API bool registerRpc(char *path, Store *request_schema, Store *response_schema, RpcImplementation *implementation)
{
	if (g_hash_table_contains(service_map, path)) {
		logWarning("Failed to register rpc because path %s is already bound", path);
		return false;
	}
	RpcService *service = createRpcService(path, request_schema, response_schema, implementation);
	g_hash_table_replace(service_map, g_strdup(path), service); 
	logInfo("Successfully registerd rpc at path: %s", path);
	return true;
}

API bool unregisterRpc(char *path)
{
	if (!g_hash_table_contains(service_map, path)) {
		logWarning("Failed to unregister rpc because path %s is not bound", path);
		return false;
	}
	g_hash_table_remove(service_map, path);
	return true;
}

API Store *callRpc(char *path, Store *request)
{
	logInfo("Calling rpc %s", path);
	RpcService *service = g_hash_table_lookup(service_map, path);
	if (service == NULL) {
		logWarning("Failed to call rpc because path %s is not bound", path);
		return NULL;
	}

	logWarning("Calling rpcs not yet implemented");
	return NULL;

	// Pseudocode:
	// 1) Validate the request store
	// 2) Fetch the implementation function and call it on request
  // 3) Validate the response store
  // 4) Return the response store
}

RpcService *createRpcService(char *path, Store *request_schema, Store *response_schema, RpcImplementation *implementation)
{
	RpcService *result = ALLOCATE_OBJECT(RpcService);
	result->path = g_strdup(path);
	result->implementation = implementation;
	result->request_schema = cloneStore(request_schema);
	result->response_schema = cloneStore(response_schema);
	return result;
}

void destroyRpcService(RpcService *rpc_service)
{
	free(rpc_service->path);
	freeStore(rpc_service->request_schema);
	freeStore(rpc_service->response_schema);
	free(rpc_service);
}
