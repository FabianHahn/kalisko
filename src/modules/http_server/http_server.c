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
#include "dll.h"

#define API
#include "http_server.h"
#include "http_parser.h"
#include "modules/event/event.h"
#include "modules/socket/poll.h"

#define FILE_NOT_FOUND_STATUS_CODE 404
#define BAD_REQUEST_STATUS_CODE 501

MODULE_NAME("http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server library which can be used to easily create http servers.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

static void clientSocketRead(void *subject, const char *event, void *data, va_list args);
static void clientAccepted(void *subject, const char *event, void *data, va_list args);
static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args);
static void tryFreeServer(HttpServer *server);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{
}

/** Struct used to map regular expressions to function which respond to HTTP requests */
typedef struct
{
	char *regexp;
	HttpRequestHandler *handler;
} RequestHandlerMapping;

static RequestHandlerMapping *createRequestHandlerMapping(char *regexp, HttpRequestHandler *handler)
{	
	RequestHandlerMapping *mapping = ALLOCATE_OBJECT(RequestHandlerMapping);
	mapping->regexp = g_strdup(regexp);
	mapping->handler = handler;
	return mapping;
}

// Takes a void pointer in order to pass it as free function to g_array (without warnings). WARNING: this does not free the pointer itself. This is the behavior request by g_array
static void freeRequestHandlerMappingContent(void *mapping)
{
	RequestHandlerMapping *rhm = mapping;
	free(rhm->regexp);
}

// A proper free function for RequestHandlerMapping
static void freeRequestHandlerMapping(RequestHandlerMapping *mapping)
{
	freeRequestHandlerMappingContent(mapping);
	free(mapping);
}

/**
 * Creates an HTTP server on the specified port. The server does not accept any connections until startHttpServer() is called. The caller takes responsibility for eventually calling destoryHttpServer() on the returned pointer.
 *
 * @param port		the port to which the new server should be bound
 * @result				the created HTTP server
 */
API HttpServer *createHttpServer(char* port)
{
	LOG_DEBUG("Creating HttpServer on port %s", port);

	HttpServer *server = ALLOCATE_OBJECT(HttpServer);
	server->state = SERVER_STATE_CREATED;
	server->open_connections = 0;
	server->server_socket = createServerSocket(port); 
	server->handler_mappings = g_array_new(false, false, sizeof(RequestHandlerMapping*));	
	g_array_set_clear_func(server->handler_mappings, &freeRequestHandlerMappingContent);

	enableSocketPolling(server->server_socket);
	attachEventListener(server->server_socket, "accept", server, &clientAccepted);
	return server;
}

/**
 * Stops and tears down the HTTP server. Waits for accepted connections to disconnect (if any) and frees all memory associated with the server.
 */
API void destroyHttpServer(HttpServer *server)
{
	LOG_DEBUG("Freeing HttpServer on port %s", server->server_socket->port);

	// Clean up the server socket
	disableSocketPolling(server->server_socket);
	detachEventListener(server->server_socket, "accept", NULL, &clientAccepted);
	freeSocket(server->server_socket); 

	server->state = SERVER_STATE_FREEING;
	tryFreeServer(server);
}

/**
 * Causes the server to start accepting connections.
 */
API bool startHttpServer(HttpServer *server)
{
	if (!connectSocket(server->server_socket)) {
		LOG_DEBUG("Unable to connect server socket on port %s", server->server_socket->port);
		return false;
	}
	LOG_DEBUG("Starting HttpServer on port %s", server->server_socket->port);
	server->state = SERVER_STATE_RUNNING;
	return true;
}

/**
 * Causes the passed request handler to be called when an HttpRequest with a matching URI comes in. Note that the caller retains ownership of all passed parameters (uri_regexp is copied).
 *
 * @param server			the server in question
 * @param uri_regexp	the regular expression used to determine whether the request matches
 * @param handler			a handler function to be called for matching requests
 */
API void registerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler)
{
	LOG_DEBUG("Mapping HTTP request handler for URIs matching %s", hierarchical_regexp);
	RequestHandlerMapping *mapping = createRequestHandlerMapping(hierarchical_regexp, handler);
	g_array_append_val(server->handler_mappings, mapping);
}

/** Returns whether or not the request has a value associated with key. */
API bool hasParameter(HttpRequest *request, char *key)
{
	return g_hash_table_contains(request->parameters, key);
}

/** Returns the value associated with key if there is one, and NULL otherwise. The caller is responsible for freeing the returned string */
API char *getParameter(HttpRequest *request, char *key)
{
	char *original = g_hash_table_lookup(request->parameters, key);
	if(original == NULL) {
		return NULL;
	} else {
		return g_strdup(original);
	}
}

/** Returns a pointer to the GHashTable representing the parameters (for advanced use such as iterating over the key-value pairs). It is the responsibility of the caller not to change the state of the GHashTable. The caller must not free the returned pointer. */
API GHashTable *getParameters(HttpRequest *request)
{
	return request->parameters;
}

static void tryFreeServer(HttpServer *server)
{
	if(server->state == SERVER_STATE_FREEING && server->open_connections == 0) {
		LOG_DEBUG("Freeing HTTP server resources");

		// Clean up all the created handler mappings. Note that this cannot be done by passing a "clear function" to the array since the regexp string of each mapping must be freed as well
		/*
		GArray *mappings = server->handler_mappings;
		for (int i = 0; i < mappings->len; ++i) {
			RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
			free(mapping->regexp);
			free(mapping);
		}
		g_array_free(mappings, true);
		*/
		g_array_free(server->handler_mappings, true);

		// Finally, clean up the server struct itself
		free(server);
	}
}

static HttpRequest *createHttpRequest(HttpServer *server)
{
	HttpRequest *request = ALLOCATE_OBJECT(HttpRequest);
	request->server = server;
	request->method = HTTP_REQUEST_METHOD_UNKNOWN;
	request->uri = NULL;
	request->hierarchical = NULL;
	request->parameters = g_hash_table_new_full(g_str_hash, g_str_equal, &free, &free);  
	request->line_buffer = g_string_new("");
	request->parsing_complete = false;
	return request;
}

static void destroyHttpRequest(HttpRequest *request)
{
	free(request->uri);
	free(request->hierarchical);
	g_hash_table_destroy(request->parameters);	// Frees all the key and value strings
	g_string_free(request->line_buffer, true);
	free(request);
}

static void clientAccepted(void *subject, const char *event, void *data, va_list args)
{
	HttpServer *server = data;
	server->open_connections++;
	HttpRequest *request = createHttpRequest(server);

	Socket *s = va_arg(args, Socket *);
	attachEventListener(s, "read", request, &clientSocketRead);
	attachEventListener(s, "disconnect", request, &clientSocketDisconnected);
	enableSocketPolling(s);
}

static void checkForNewLine(HttpRequest *request)
{
	char *message = request->line_buffer->str;
	if (strstr(message, "\n") == NULL) {
		// No newline in the string, just do nothing
		return;
	}

	g_string_free(request->line_buffer, false);
	char **parts = g_strsplit(message, "\n", 0);
	int count = 0;
	for(char **iter = parts; *iter != NULL; ++iter) {
		++count;
	}

	// Put the last part back into the buffer and process all complete lines
	request->line_buffer = g_string_new(parts[count - 1]);
	for(int i = 0; i < count - 1; i++) {
		// Remove all leading and trailing \r characters. In lack of a removeTrailingChars function, we first transform \r into spaces and then call strstrip().
		g_strdelimit(parts[i], "\r", ' ');
		g_strstrip(parts[i]);
		parseLine(request, parts[i]);
	}

	g_strfreev(parts);
	free(message);
}

/** Sends the provided response to the client */
static void sendResponse(HttpResponse *response, Socket *client)
{
	// TODO: factor out constant strings, ad support for proper return codes
	GString *content = g_string_new(response->content);
	GString *answer = g_string_new("");

	// 200 OK
	g_string_append(answer, "HTTP/1.0 200 OK \r\n");
	if(response->content != NULL) {
		g_string_append_printf(answer, "Content-Type: text/html\r\nContent-length: %lu\r\n", (unsigned long)content->len);	
	}
	g_string_append(answer, "\r\n");
	if(response->content != NULL) {
		g_string_append(answer, content->str);
	}

	socketWriteRaw(client, answer->str, answer->len * sizeof(char));
	g_string_free(content, true);
	g_string_free(answer, true);
}

/** Sends to the client a response which consists only of the status code and has no other content */
static void sendStatusResponse(int code, Socket *client)
{
	HttpResponse response;
	response.content = NULL;
	response.status_code = code;
	sendResponse(&response, client);
}

/** Reads the data in request and sends an appropriate response to the client (only if the request is well-formed) */
static void handleRequest(HttpRequest *request, Socket *client)
{
	if(request->method == HTTP_REQUEST_METHOD_UNKNOWN || request->hierarchical == NULL) {
		LOG_DEBUG("Could not parse request, responding with bad request");
		sendStatusResponse(BAD_REQUEST_STATUS_CODE, client);
		return;
	}

	// Go through all handlers and execute the first one which matches the requested URI
	HttpServer *server = request->server;
	GArray *mappings = server->handler_mappings;
	for (int i = 0; i < mappings->len; ++i) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
		if(g_regex_match_simple(mapping->regexp, request->hierarchical, 0, 0)) {
			HttpResponse response;
			mapping->handler(request, &response);
			sendResponse(&response, client);
			return;
		}
	}

	// If we got this far, there is no handler registered for this request
	LOG_DEBUG("No handler for hierarchical part %s, responding with file not found", request->hierarchical);
	sendStatusResponse(FILE_NOT_FOUND_STATUS_CODE, client);
}

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args)
{	
	Socket *client_socket = subject;
	HttpRequest *request = data;
	HttpServer *server = request->server;
	
	detachEventListener(client_socket, "read", request, &clientSocketRead);
	detachEventListener(client_socket, "disconnect", request, &clientSocketDisconnected);
	freeSocket(client_socket);
	destroyHttpRequest(request);

	server->open_connections--;
	tryFreeServer(server);	// This might have been the last request, attempt to free the server
}

static void clientSocketRead(void *subject, const char *event, void *data, va_list args)
{
	char *message = va_arg(args, char *);
	HttpRequest *request = data;

	g_string_append(request->line_buffer, message);
	checkForNewLine(request);

	if(request->parsing_complete) {
		handleRequest(request, subject);
	}
}
