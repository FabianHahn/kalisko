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
#include "modules/rpc/line_server.h"
#include "modules/store/clone.h"
#include "modules/store/parse.h"
#include "modules/store/store.h"
#include "modules/store/write.h"
#define API

#define RPC_PORT "8889"
#define REQUEST_FIRST_LINE_REGEX "^rpc[ ]+(?<METHOD>list|call)([ ]+(?<PATH>.*))?$"

MODULE_NAME("rpc");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides an easy way to implement an rpc interface built on top of stores.");
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
	RpcImplementation implementation;
} RpcService;

/** Stores the mapping from path to the correspinding services. **/
static GHashTable *service_map;

/** Stores the central rpc server. */
static LineServer *line_server;

static void lineServerCallback(LineServerClient *client);
static void processRequest(LineServerClient *client, int empty_line_index, GString *response);
static void processListRequest(LineServerClient *client, char *path, GString *response);
static void processCallRequest(LineServerClient *client, char *path, GString *response);

static bool matchFirstLine(char *first_line, GString *method, GString *path);
static unsigned int findMatchingServices(GHashTable *service_map, char *path, GPtrArray *results);

static RpcService *createRpcService(char *path, Store *requestSchema, Store *responseSchema, RpcImplementation implementation);
static void destroyRpcService(RpcService *rpcService);

MODULE_INIT
{
	service_map = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) &destroyRpcService);
	line_server = startLineServer(RPC_PORT, (LineServerCallback) &lineServerCallback);
	return true;
}

MODULE_FINALIZE
{
	stopLineServer(line_server);
	g_hash_table_destroy(service_map);
}

/***************************************************************/
/* TODO: Remove this once actual store validation is available */
/***************************************************************/
bool validateStoreByStoreSchema(Store *store, Store *schema)
{
	return true;
}

API bool registerRpc(char *path, Store *request_schema, Store *response_schema, RpcImplementation implementation)
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

	if (service->request_schema != NULL && !validateStoreByStoreSchema(request, service->request_schema)) {
		logWarning("Request store validation failed");
		return NULL;
	}
	Store *response = service->implementation(request);
	if (service->response_schema != NULL && !validateStoreByStoreSchema(response, service->response_schema)) {
		logWarning("Response store validation failed");
		freeStore(response);
		return NULL;
	}
	return response;
}

void lineServerCallback(LineServerClient *client)
{
	// Locate the first empty line, if any.
	int empty_line_index = -1;
	for (int i = 0; i < client->lines->len; ++i) {
		char *line = g_ptr_array_index(client->lines, i);
		if (strlen(line) == 0) {
			empty_line_index = i;
			break;
		}
	}

	if (empty_line_index != -1) {
		GString *response = g_string_new("");
		processRequest(client, empty_line_index, response);
		sendToLineServerClient(client, response->str);
		g_string_free(response, true);
		disconnectLineServerClient(client);
	}
}

void processRequest(LineServerClient *client, int empty_line_index, GString *response)
{
	if (empty_line_index == 0) {
		g_string_append(response, "Failed to process rpc, empty request\n");
		return;
	}
	char *first_line = g_ptr_array_index(client->lines, 0);

	GString *method = g_string_new("");
	GString *path = g_string_new("");
	if (!matchFirstLine(first_line, method, path)) {
		g_string_free(method, true);
		g_string_free(path, true);
		g_string_append(response, "Invalid request\n");
		return;
	}

	if (g_strcmp0("list", method->str) == 0) {
		processListRequest(client, path->str, response);
	} else if (g_strcmp0("call", method->str) == 0) {
		processCallRequest(client, path->str, response);
	}

	g_string_free(method, true);
	g_string_free(path, true);
}

void processCallRequest(LineServerClient *client, char *path, GString *response)
{
	logInfo("Processing rpc call for path: %s", path);

	// TODO: Figure out if stores can be parsed as a stream. For now, reconstruct
	// serialized store string from the lines and parse the whole thing at once.
	GString *serialized_request_store = g_string_new("");
	for (int i = 1; i < client->lines->len; ++i) {
		char *line = g_ptr_array_index(client->lines, i);
		g_string_append_printf(serialized_request_store, "%s\n", line);
	}
	Store *request_store = parseStoreString(serialized_request_store->str);
	g_string_free(serialized_request_store, true);

	Store *response_store = callRpc(path, request_store);
	freeStore(request_store);

	if (response_store == NULL) {
		g_string_append(response, "Failed to execute rpc\n");
	} else {
		GString *serialized_response_store = writeStoreGString(response_store);
		g_string_append_printf(response, "%s\n", serialized_response_store->str);
		g_string_free(serialized_response_store, true);
		freeStore(response_store);
	}
}

void processListRequest(LineServerClient *client, char *path, GString *response)
{
	logInfo("Processing rpc list for path: %s", path);

	GPtrArray *matching = g_ptr_array_new_with_free_func(&free);
	if (findMatchingServices(service_map, path, matching) == 0) {
		g_string_append(response, "No matching services\n");
	} else {
		for (int i = 0; i < matching->len; ++i) {
			char *service = g_ptr_array_index(matching, i);
			g_string_append_printf(response, "%s\n", service);
		}
	}
	g_ptr_array_free(matching, true);
}

// Returns true iff first_line valid. If true, populates method with the parsed
// method and path with the parsed path. If no path is found, then path is
// populated with the empty string.
bool matchFirstLine(char *first_line, GString *method, GString *path)
{
	GRegex *regex = g_regex_new(REQUEST_FIRST_LINE_REGEX, 0, 0, NULL);
	GMatchInfo *match_info;

	bool matched = g_regex_match(regex, first_line, 0, &match_info);
	if (matched) {
		char *matched_method = g_match_info_fetch_named(match_info, "METHOD");
		g_string_assign(method, matched_method);
		free(matched_method);

		char *matched_path = g_match_info_fetch_named(match_info, "PATH");
		if (matched_path == NULL) {
			g_string_assign(path, "");
		} else {
			g_string_assign(path, matched_path);
		}
		free(matched_path);
	}

	g_match_info_free(match_info);
	g_regex_unref(regex);
	return matched;
}

// Add all services matching path to results. The caller is responsible for
// freeing all items added to services. Returns the number of services added.
// Note that if path is the empty string, all services are matched.
unsigned int findMatchingServices(GHashTable *service_map, char *path, GPtrArray *results)
{
	GHashTableIter iter;
	gpointer key, value;

	unsigned int added = 0;
	g_hash_table_iter_init(&iter, service_map);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		if (g_str_has_prefix(key, path)) {
			g_ptr_array_add(results, strdup(key));
			++added;
		}
	}
	return added;
}

RpcService *createRpcService(char *path, Store *request_schema, Store *response_schema, RpcImplementation implementation)
{
	RpcService *result = ALLOCATE_OBJECT(RpcService);
	result->path = g_strdup(path);
	result->implementation = implementation;
	result->request_schema = request_schema == NULL ? NULL : cloneStore(request_schema);
	result->response_schema = response_schema == NULL ? NULL : cloneStore(response_schema);
	return result;
}

void destroyRpcService(RpcService *rpc_service)
{
	free(rpc_service->path);
	if (rpc_service->request_schema != NULL) {
		freeStore(rpc_service->request_schema);
	}
	if (rpc_service->response_schema != NULL) {
		freeStore(rpc_service->response_schema);
	}
	free(rpc_service);
}
