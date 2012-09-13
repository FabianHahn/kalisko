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
 * Creates an HTTP server on the specified port. The caller takes responsibility for freeing the returned pointer
 *
 * @param port    the port to which the new server should be bound
 * @result				the create HTTP server
 */
API HttpServer *createHttpServer(char* port)
{
	HttpServer *server = ALLOCATE_OBJECT(HttpServer);
	server->server_socket = createServerSocket(port);  // TODO: possibly provide a cleanup method which destroys the socket
  enableSocketPolling(server->server_socket);
  attachEventListener(server->server_socket, "accept", NULL, &listener_serverSocketAccept);

	LOG_DEBUG("Created HttpServer on port %s", port);
  return server;
}

API void registerRequestHandler(char *url_regexp, HttpRequestHandler *handler)
{
  // TODO: figure out how best to store the request handlers in the HTTP server
}

/**
 * Causes the server to start accepting connections
 */
API bool startHttpServer(HttpServer *server)
{
  if (!server->server_socket) {
    return false;
  }

  if (connectSocket(server->server_socket)) {
    LOG_DEBUG("Started HttpServer on port %s", server->server_socket->port);
    return true;
  }
  return false;
}

/**
 * Causes the server to stop accepting connections
 */
API bool stopHttpServer(HttpServer *server)
{
  if (!server->server_socket) {
    return false;
  }
  return disconnectSocket(server->server_socket);
}

static void listener_serverSocketAccept(void *subject, const char *event, void *data, va_list args)
{
	Socket *srv = subject;
	Socket *s = va_arg(args, Socket *);
	if(srv == NULL) {
		return;
	};

  GString *content = g_string_new("<!DOCTYPE HTML>\r\n<html><head></head><body>Hello world</body></html>\r\n");
  GString *response = g_string_new("");
  g_string_append_printf(response, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-length: %lu\r\nConnection: close\r\n\r\n", content->len);
  g_string_append(response, content->str);

	socketWriteRaw(s, response->str, response->len * sizeof(char));
	disconnectSocket(s);

  g_string_free(content, true);
  g_string_free(response, true);
}
