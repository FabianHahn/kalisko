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

#include <stdarg.h>
#include <glib.h>
#include "dll.h"
#include "modules/event/event.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#define API
#include "http_server.h"
#include "http_parser.h"

#define OK_STATUS_STRING "200 OK"
#define FILE_NOT_FOUND_STATUS_STRING "404 Not Found"
#define BAD_REQUEST_STATUS_STRING "400 Bad Request"

// TODO: Observed bug: if you create and start multiple HTTP server, all request go to the first one started.
// The working theory is that this is a bug in the socket module (and not in http_server). Need to investigate.

MODULE_NAME("http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server library which can be used to easily create http servers.");
MODULE_VERSION(0, 1, 5);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

/** Struct used to map regular expressions to function which respond to HTTP requests */
typedef struct
{
	char *regexp;
	HttpRequestHandler *handler;
	void *userdata;
} RequestHandlerMapping;

/** Struct used to map requests to the server on which they came in */
typedef struct
{
	HttpServer *server;
	HttpRequest *request;
} ServerRequestMapping;

static RequestHandlerMapping *createRequestHandlerMapping(char *regexp, HttpRequestHandler *handler, void *userdata);
static void freeRequestHandlerMappingContent(void *mapping);
static ServerRequestMapping *createServerRequestMapping(HttpServer *server, HttpRequest *request);
static void freeServerRequestMapping(ServerRequestMapping *mapping);

static void tryFreeServer(HttpServer *server);
static void clientAccepted(void *subject, const char *event, void *data, va_list args);
static void processAvailableLines(HttpRequest *request);
static bool sendResponse(Socket *client, HttpResponse *response);
static void handleAndRespond(Socket *client, HttpServer *server, HttpRequest *request);
static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args);
static void clientSocketRead(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

API HttpServer *createHttpServer(char* port)
{
	logInfo("Creating HttpServer on port %s", port);

	HttpServer *server = ALLOCATE_OBJECT(HttpServer);
	server->state = SERVER_STATE_CREATED;
	server->open_connections = 0;
	server->server_socket = createServerSocket(port);
	server->handler_mappings = g_array_new(false, false, sizeof(RequestHandlerMapping*));
	g_array_set_clear_func(server->handler_mappings, &freeRequestHandlerMappingContent);

	attachEventListener(server->server_socket, "accept", server, &clientAccepted);
	return server;
}

API void destroyHttpServer(HttpServer *server)
{
	logInfo("Freeing HttpServer on port %s", server->server_socket->port);

	// Clean up the server socket
	disableSocketPolling(server->server_socket);
	detachEventListener(server->server_socket, "accept", server, &clientAccepted);
	freeSocket(server->server_socket);

	server->state = SERVER_STATE_FREEING;
	tryFreeServer(server);
}

API bool startHttpServer(HttpServer *server)
{
	if(!connectSocket(server->server_socket)) {
		logInfo("Unable to connect server socket on port %s", server->server_socket->port);
		return false;
	}
	logInfo("Starting HttpServer on port %s", server->server_socket->port);
	server->state = SERVER_STATE_RUNNING;
	enableSocketPolling(server->server_socket);
	return true;
}

API void registerHttpServerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata)
{
	logInfo("Registering HTTP request handler for URIs matching %s", hierarchical_regexp);
	RequestHandlerMapping *mapping = createRequestHandlerMapping(hierarchical_regexp, handler, userdata);
	g_array_append_val(server->handler_mappings, mapping);
}

API void unregisterHttpServerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata)
{
	logInfo("Unregistering HTTP request handler for URIs matching %s", hierarchical_regexp);
	GArray *mappings = server->handler_mappings;

	int match_index = -1;
	for(int i = 0; i < mappings->len; ++i) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
		if(!strcmp(mapping->regexp, hierarchical_regexp) && mapping->handler == handler && mapping->userdata == userdata) {
			if(match_index != -1) {
				logInfo("Unregistering found multiple matches, using last one");
			}
			match_index = i;
		}
	}

	if(match_index != -1) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, match_index);
		freeRequestHandlerMappingContent(mapping);
		g_array_remove_index(mappings, match_index); // Frees the mapping struct itself
	}
}

API HttpRequest *createHttpRequest()
{
	HttpRequest *request = ALLOCATE_OBJECT(HttpRequest);
	request->method = HTTP_REQUEST_METHOD_UNKNOWN;
	request->uri = NULL;
	request->hierarchical = NULL;
	request->parameters = g_hash_table_new_full(g_str_hash, g_str_equal, &free, &free);
	request->line_buffer = g_string_new("");
	request->content_length = -1;
	request->got_empty_line = false;
	return request;
}

API void destroyHttpRequest(HttpRequest *request)
{
	free(request->uri);
	free(request->hierarchical);
	g_hash_table_destroy(request->parameters); // Frees all the key and value strings
	g_string_free(request->line_buffer, true);
	free(request);
}

API bool checkHttpRequestParameter(HttpRequest *request, char *key)
{
	return g_hash_table_contains(request->parameters, key);
}

API char *getHttpRequestParameter(HttpRequest *request, char *key)
{
	char *original = g_hash_table_lookup(request->parameters, key);
	if(original == NULL) {
		return NULL;
	} else {
		return g_strdup(original);
	}
}

API void appendHttpResponseContent(HttpResponse *response, char *content, ...)
{
	va_list va;
	va_start(va, content);

	g_string_append_vprintf(response->content, content, va);
}

API void clearHttpResponseContent(HttpResponse *response)
{
	g_string_assign(response->content, "");
}

API HttpResponse *createHttpResponse(const char *status, const char *content)
{
	HttpResponse *response = ALLOCATE_OBJECT(HttpResponse);
	response->status = strdup(status);
	response->content = g_string_new(content);
	return response;
}

API void destroyHttpResponse(HttpResponse *response)
{
	free(response->status);
	g_string_free(response->content, true);
	free(response);
}

API HttpResponse *handleHttpRequest(HttpServer *server, HttpRequest *request)
{
	if(request->method == HTTP_REQUEST_METHOD_UNKNOWN || request->hierarchical == NULL) {
		logInfo("Could not parse request, returning bad request");
		return createHttpResponse(BAD_REQUEST_STATUS_STRING, BAD_REQUEST_STATUS_STRING);
	}

	// Go through all handlers and execute the first one which matches the requested URI
	GArray *mappings = server->handler_mappings;
	for(int i = 0; i < mappings->len; ++i) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
		if(g_regex_match_simple(mapping->regexp, request->hierarchical, 0, 0)) {
			logInfo("%s matches %s", request->hierarchical, mapping->regexp);
			HttpResponse *response = createHttpResponse(OK_STATUS_STRING, "");
			if(mapping->handler(request, response, mapping->userdata)) {
				return response;
			}
			destroyHttpResponse(response);
		} else {
			logTrace("%s does not match %s", request->hierarchical, mapping->regexp);
		}
	}

	logInfo("No handler for hierarchical part %s, returning file not found", request->hierarchical);
	return createHttpResponse(FILE_NOT_FOUND_STATUS_STRING, FILE_NOT_FOUND_STATUS_STRING);
}

static RequestHandlerMapping *createRequestHandlerMapping(char *regexp, HttpRequestHandler *handler, void *userdata)
{
	RequestHandlerMapping *mapping = ALLOCATE_OBJECT(RequestHandlerMapping);

	// Add leading and trailing metasymbols in order to force an exact match.
	mapping->regexp = g_strdup_printf("^%s$", regexp);
	mapping->handler = handler;
	mapping->userdata = userdata;
	return mapping;
}

/**
 * Takes a void pointer in order to pass it as free function to g_array (without warnings). WARNING: this does not free the pointer itself. This is the behavior requested by g_array
 */
static void freeRequestHandlerMappingContent(void *mapping)
{
	RequestHandlerMapping *rhm = mapping;
	free(rhm->regexp);
}

static ServerRequestMapping *createServerRequestMapping(HttpServer *server, HttpRequest *request)
{
	ServerRequestMapping *mapping = ALLOCATE_OBJECT(ServerRequestMapping);
	mapping->server = server;
	mapping->request = request;
	return mapping;
}

static void freeServerRequestMapping(ServerRequestMapping *mapping)
{
	free(mapping);
}

static void tryFreeServer(HttpServer *server)
{
	if(server->state == SERVER_STATE_FREEING && server->open_connections == 0) {
		g_array_free(server->handler_mappings, true); // Frees all RequestHandlerMapping structs
		free(server);
	}
}

static void clientAccepted(void *subject, const char *event, void *data, va_list args)
{
	HttpServer *server = data;
	HttpRequest *request = createHttpRequest();
	ServerRequestMapping *mapping = createServerRequestMapping(server, request);
	server->open_connections++;

	Socket *s = va_arg(args, Socket *);
	attachEventListener(s, "read", mapping, &clientSocketRead);
	attachEventListener(s, "disconnect", mapping, &clientSocketDisconnected);
	enableSocketPolling(s);
}

/**
 * Reads the buffer line by line until an empty line is parsed. After that, does nothing.
 */
static void processAvailableLines(HttpRequest *request)
{
	char *message = request->line_buffer->str;

	if(request->got_empty_line) {
		// Accumulating content body in buffer, do nothing
		return;
	}

	if(strstr(message, "\n") == NULL) {
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
		if(request->got_empty_line) {
			// Empty line occurred, switch to accumulating content body, just put the string back into the buffer
			g_string_append(request->line_buffer, message);
		} else {
			// Remove all leading and trailing \r characters. In lack of a removeTrailingChars function, we first transform \r into spaces and then call strstrip()
			g_strdelimit(parts[i], "\r", ' ');
			g_strstrip(parts[i]);
			parseHttpRequestLine(request, parts[i]);
		}
	}

	g_strfreev(parts);
	free(message);
}

/**
 * Sends the provided response to the client
 *
 * @param client			the client socket to send the response to
 * @param response			the HTTP response to send to the client
 * @result					true if successful
 */
static bool sendResponse(Socket *client, HttpResponse *response)
{
	GString *answer = g_string_new("");
	g_string_append_printf(answer, "HTTP/1.0 %s \r\nContent-Type: text/html; charset=utf-8\r\nContent-length: %lu\r\n\r\n%s", response->status, (unsigned long) response->content->len, response->content->str);
	bool result = socketWriteRaw(client, answer->str, answer->len * sizeof(char));
	g_string_free(answer, true);

	return result;
}

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args)
{
	Socket *client_socket = subject;
	ServerRequestMapping *mapping = data;
	HttpRequest *request = mapping->request;
	HttpServer *server = mapping->server;

	detachEventListener(client_socket, "read", mapping, &clientSocketRead);
	detachEventListener(client_socket, "disconnect", mapping, &clientSocketDisconnected);
	freeSocket(client_socket);
	destroyHttpRequest(request);
	freeServerRequestMapping(mapping);

	server->open_connections--;
	tryFreeServer(server); // This might have been the last request, attempt to free the server
}

static void handleAndRespond(Socket *client, HttpServer *server, HttpRequest *request)
{
	HttpResponse *response = handleHttpRequest(server, request);
	sendResponse(client, response);
	destroyHttpResponse(response);
}

static void clientSocketRead(void *subject, const char *event, void *data, va_list args)
{
	char *message = va_arg(args, char *);
	ServerRequestMapping *mapping = data;
	HttpRequest *request = mapping->request;
	HttpServer *server = mapping->server;

	g_string_append(request->line_buffer, message);
	processAvailableLines(request);

	if(!request->got_empty_line) {
		// Still no empty line, so nothing to do yet
		return;
	}

	if(request->method == HTTP_REQUEST_METHOD_GET) {
		// Empty line in GET request indicates the end of the request.
		handleAndRespond(subject, server, request);
		return;
	}

	if(request->method == HTTP_REQUEST_METHOD_POST && request->line_buffer->len >= request->content_length) {
		parseHttpRequestBody(request, request->line_buffer->str);
		handleAndRespond(subject, server, request);
		return;
	}
}
