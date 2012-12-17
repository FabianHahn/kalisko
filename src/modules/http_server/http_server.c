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

#define API
#include "http_server.h"
#include "http_parser.h"
#include "modules/event/event.h"
#include "modules/socket/poll.h"

#define OK_STATUS_STRING "200 OK"
#define FILE_NOT_FOUND_STATUS_STRING "404 Not Found"
#define BAD_REQUEST_STATUS_STRING "400 Bad Request"

MODULE_NAME("http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server library which can be used to easily create http servers.");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

/** Struct used to map regular expressions to function which respond to HTTP requests */
typedef struct
{
	char *regexp;
	HttpRequestHandler *handler;
	void *userdata;
} RequestHandlerMapping;


static RequestHandlerMapping *createRequestHandlerMapping(char *regexp, HttpRequestHandler *handler, void *userdata);
static void freeRequestHandlerMappingContent(void *mapping);
static void tryFreeServer(HttpServer *server);
static HttpRequest *createHttpRequest(HttpServer *server);
static void destroyHttpRequest(HttpRequest *request);
static void clientAccepted(void *subject, const char *event, void *data, va_list args);
static void processAvailableLines(HttpRequest *request);
static bool sendResponse(Socket *client, HttpResponse *response);
static bool sendStatusResponse(Socket *client, const char *status);
static void handleRequest(Socket *client, HttpRequest *request);
static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args);
static void clientSocketRead(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Creates an HTTP server on the specified port. The server does not accept any connections until
 * startHttpServer() is called. The caller takes responsibility for eventually calling
 * destoryHttpServer() on the returned pointer.
 *
 * @param port			the port to which the new server should be bound
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
 * Stops and tears down the HTTP server. Waits for accepted connections to disconnect (if any)
 * and frees all memory associated with the server.
 */
API void destroyHttpServer(HttpServer *server)
{
	LOG_DEBUG("Freeing HttpServer on port %s", server->server_socket->port);

	// Clean up the server socket
	disableSocketPolling(server->server_socket);
	detachEventListener(server->server_socket, "accept", server, &clientAccepted);
	freeSocket(server->server_socket);

	server->state = SERVER_STATE_FREEING;
	tryFreeServer(server);
}

/**
 * Causes the server to start accepting connections.
 */
API bool startHttpServer(HttpServer *server)
{
	if(!connectSocket(server->server_socket)) {
		LOG_DEBUG("Unable to connect server socket on port %s", server->server_socket->port);
		return false;
	}
	LOG_DEBUG("Starting HttpServer on port %s", server->server_socket->port);
	server->state = SERVER_STATE_RUNNING;
	return true;
}

/**
 * Causes the passed request handler to be called when an HttpRequest with a matching URI comes in.
 * In order to determine the matching precedence, matches are tested in the order in which they were
 * registered. Note that the caller retains ownership of all passed parameters (uri_regexp is copied).
 *
 * @param server			the server in question
 * @param uri_regexp		the regular expression used to determine whether the request matches
 * @param handler			a handler function to be called for matching requests
 * @param userdata			custom userdata
 */
API void registerHttpServerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata)
{
	LOG_DEBUG("Mapping HTTP request handler for URIs matching %s", hierarchical_regexp);
	RequestHandlerMapping *mapping = createRequestHandlerMapping(hierarchical_regexp, handler, userdata);
	g_array_append_val(server->handler_mappings, mapping);
}

/**
 * Returns whether or not the request has a value associated with key.
 *
 * @param request			the request to check
 * @param key				the key to check
 * @result					true if such a parameter exists
 */
API bool checkHttpRequestParameter(HttpRequest *request, char *key)
{
	return g_hash_table_contains(request->parameters, key);
}

/**
 * Returns the value associated with key if there is one, and NULL otherwise. The caller is responsible
 * for freeing the returned string.
 *
 * @param request			the request for which the parameter should be returned
 * @param key				the key of the parameter that should be returned
 * @result					the returned parameter of NULL on failure
 */
API char *getHttpRequestParameter(HttpRequest *request, char *key)
{
	char *original = g_hash_table_lookup(request->parameters, key);
	if(original == NULL) {
		return NULL;
	} else {
		return g_strdup(original);
	}
}

/**
 * Create a HTTP response object
 *
 * @param status			the status string to initialize the HTTP response with
 * @param content			the content string to initialize the HTTP response with
 * @result					the created HTTP response object
 */
API HttpResponse *createHttpResponse(const char *status, const char *content)
{
	HttpResponse *response = ALLOCATE_OBJECT(HttpResponse);
	response->status = strdup(status);
	response->content = g_string_new(content);

	return response;
}

/**
 * Adds content to a HTTP response object
 *
 * @param response			the HTTP response to add the content to
 * @param content			the printf-style content to append to the response
 */
API void appendHttpResponseContent(HttpResponse *response, char *content, ...)
{
	va_list va;
	va_start(va, content);

	g_string_append_vprintf(response->content, content, va);
}

/**
 * Resets the content of the response to the empty string
 *
 * @param response			the HTTP response to clear
 */
API void clearHttpResponseContent(HttpResponse *response)
{
	g_string_assign(response->content, "");
}

/**
 * Frees a HTTP response object
 *
 * @param response			the HTTP response to free
 */
API void freeHttpResponse(HttpResponse *response)
{
	free(response->status);
	g_string_free(response->content, true);
	free(response);
}

static RequestHandlerMapping *createRequestHandlerMapping(char *regexp, HttpRequestHandler *handler, void *userdata)
{
	RequestHandlerMapping *mapping = ALLOCATE_OBJECT(RequestHandlerMapping);
	mapping->regexp = g_strdup(regexp);
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

static void tryFreeServer(HttpServer *server)
{
	if(server->state == SERVER_STATE_FREEING && server->open_connections == 0) {
		g_array_free(server->handler_mappings, true); // Frees all RequestHandlerMapping structs
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
	request->content_length = -1;
	request->got_empty_line = false;
	return request;
}

static void destroyHttpRequest(HttpRequest *request)
{
	free(request->uri);
	free(request->hierarchical);
	g_hash_table_destroy(request->parameters); // Frees all the key and value strings
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

/**
 * Sends to the client a response which consists of the status string in the header and in the body
 *
 * @param client			the client socket to send the status response to
 * @param status			the status string to send
 * @result					true if successful
 */
static bool sendStatusResponse(Socket *client, const char *status)
{
	HttpResponse *response = createHttpResponse(status, status);
	bool result = sendResponse(client, response);
	freeHttpResponse(response);

	return result;
}

/**
 * Reads the data in request and sends an appropriate response to the client (only if the request is well-formed)
 */
static void handleRequest(Socket *client, HttpRequest *request)
{
	if(request->method == HTTP_REQUEST_METHOD_UNKNOWN || request->hierarchical == NULL) {
		LOG_DEBUG("Could not parse request, responding with bad request");
		sendStatusResponse(client, BAD_REQUEST_STATUS_STRING);
		return;
	}

	// Go through all handlers and execute the first one which matches the requested URI
	HttpServer *server = request->server;
	GArray *mappings = server->handler_mappings;
	bool handled = false;
	for(int i = 0; !handled && i < mappings->len; ++i) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
		if(g_regex_match_simple(mapping->regexp, request->hierarchical, 0, 0)) {
			LOG_DEBUG("%s matches %s", mapping->regexp, request->hierarchical);
			HttpResponse *response = createHttpResponse(OK_STATUS_STRING, "");
			if(mapping->handler(request, response, mapping->userdata)) {
				sendResponse(client, response);
				handled = true;
			}
			freeHttpResponse(response);
		}
	}

	if(!handled) {
		LOG_DEBUG("No handler for hierarchical part %s, responding with file not found", request->hierarchical);
		sendStatusResponse(client, FILE_NOT_FOUND_STATUS_STRING);
	}
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
	tryFreeServer(server); // This might have been the last request, attempt to free the server
}

static void clientSocketRead(void *subject, const char *event, void *data, va_list args)
{
	char *message = va_arg(args, char *);
	HttpRequest *request = data;

	g_string_append(request->line_buffer, message);
	processAvailableLines(request);

	if(!request->got_empty_line) {
		// Still no empty line, so nothing to do yet
		return;
	}

	if(request->method == HTTP_REQUEST_METHOD_GET) {
		// Empty line in GET request indicates the end of the request.
		handleRequest(subject, request);
		return;
	}

	if(request->method == HTTP_REQUEST_METHOD_POST && request->line_buffer->len >= request->content_length) {
		parseHttpRequestBody(request, request->line_buffer->str);
		handleRequest(subject, request);
		return;
	}
}
