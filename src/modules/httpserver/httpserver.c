/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
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
#include "httpserver.h"
#include "modules/event/event.h"
#include "modules/socket/poll.h"

MODULE_NAME("httpserver");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server library which can be used to easily create http servers.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_serverSocketAccept(void *subject, const char *event, void *data, va_list args);

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

/**
 * Creates an HTTP server on the specified port. The server does not accept any connections until startHttpServer() is called. The caller takes responsibility for eventually calling freeHttpServer() on the returned pointer.
 *
 * @param port    the port to which the new server should be bound
 * @result				the created HTTP server
 */
API HttpServer *createHttpServer(char* port)
{
  LOG_DEBUG("Creating HttpServer on port %s", port);

	HttpServer *server = ALLOCATE_OBJECT(HttpServer);
	server->server_socket = createServerSocket(port); 
  server->handler_mappings = g_array_new(FALSE, FALSE, sizeof(RequestHandlerMapping*));	
  return server;
}

/**
 * Stops and tears down the internals of an HTTP server and releases the associated memory. Also frees the server struct itself.
 */
API void freeHttpServer(HttpServer *server)
{
  LOG_DEBUG("Freeing HttpServer on port %s", server->server_socket->port);

  // Clean up the server socket
  disconnectSocket(server->server_socket);
  freeSocket(server->server_socket); 

  // Clean up all the created handler mappings
  GArray *mappings = server->handler_mappings;
  for (int i = 0; i < mappings->len; ++i) {
    RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
    free(mapping->regexp);
    free(mapping);
  }
  free(mappings);

  // Finallt, clean up the server struct itself
  free(server);
}

/**
 * Causes the server to start accepting connections.
 */
API bool startHttpServer(HttpServer *server)
{
  enableSocketPolling(server->server_socket);
  attachEventListener(server->server_socket, "accept", NULL, &listener_serverSocketAccept);

  if (!connectSocket(server->server_socket)) {
    LOG_DEBUG("Unable to connect server socket on port %s", server->server_socket->port);
    return false;
  }
  LOG_DEBUG("Starting HttpServer on port %s", server->server_socket->port);
  return true;
}

/**
 * Causes the passed request handler to be called when an HttpRequest with a matching URL comes in. Note that the caller retains ownership of all passed parameters (url_regexp is copied).
 *
 * @param server      the server in question
 * @param url_regexp  the regular expression used to determine whether the request matches
 * @param handler     a handler function to be called for matching requests
 */
API void registerRequestHandler(HttpServer *server, char *url_regexp, HttpRequestHandler *handler)
{
  LOG_DEBUG("Mapping HTTP request handler for URLs matching %s.", url_regexp);

  RequestHandlerMapping *mapping = ALLOCATE_OBJECT(RequestHandlerMapping);
  GString *copy = g_string_new(url_regexp);
  mapping->regexp = g_string_free(copy, FALSE);
  mapping->handler = handler;

  g_array_append_val(server->handler_mappings, mapping);
}

static void listener_serverSocketAccept(void *subject, const char *event, void *data, va_list args)
{
	Socket *srv = subject;
	Socket *s = va_arg(args, Socket *);
	if(srv == NULL || s == NULL) {
		return;
	};

  GString *content = g_string_new("<!DOCTYPE HTML>\r\n<html><head></head><body>Hello world</body></html>\r\n");
  GString *response = g_string_new("");
  g_string_append_printf(response, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-length: %lu\r\nConnection: close\r\n\r\n%s", content->len, content->str);

	socketWriteRaw(s, response->str, response->len * sizeof(char));
	disconnectSocket(s);  // TODO: free the socket, check API for how to do that cleanly

  g_string_free(content, true);
  g_string_free(response, true);
}
