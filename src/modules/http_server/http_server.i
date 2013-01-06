/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2012, Google Inc.
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

#ifndef HTTPSERVER_HTTPSERVER_H
#define HTTPSERVER_HTTPSERVER_H

#include <glib.h>
#include <stdarg.h>
#include "modules/socket/socket.h"

/* Forward declaration to allow HttpRequest to reference a server */
struct HttpServerStruct;

/**
 * Enum to represent the types of http request which can come in
 */
typedef enum
{
	HTTP_REQUEST_METHOD_UNKNOWN,
	HTTP_REQUEST_METHOD_GET,
	HTTP_REQUEST_METHOD_POST
} HttpRequestMethod;

/**
 * Struct to represent an HTTP request
 */
typedef struct
{
	/** The server which accepted the client with this request */
	struct HttpServerStruct *server;
	/** Represents the method of this request */
	HttpRequestMethod method;
	/** Store the entire URI of the request in its raw form (without unescaping), e.g. /the/hierarchical/part?key=value&foo=bar */
	char *uri;
	/** Stores the hierarchical part of the URI, e.g. /the/hierarchical/part */
	char *hierarchical;
	/** Stores the parameters of the request */
	GHashTable *parameters;
	/** Used as intermediate storage for incomplete lines coming out of the socket stream */
	GString *line_buffer;
	/** Stores the body content length (only applicable to requests with a body) */
	int content_length;
	/** Stores whether an empty line has been seen for this request */
	bool got_empty_line;
} HttpRequest;

/**
 * Struct to represent an HTTP response
 */
typedef struct
{
	/** Contains the variable part of the status line, for instance "200 OK" */
	char *status;
	/** Contains the string sent to the client as response body */
	GString *content;
} HttpResponse;

/** 
 * A type of function which can respond to Http requests by populating a response struct
 */
typedef bool (HttpRequestHandler)(HttpRequest *request, HttpResponse *response, void *userdata);

typedef enum
{
	/** State between creation and startHttpServer call. The server should be configured in this state */
	SERVER_STATE_CREATED,
	/** State between calling startHttpServer and freeHttpServer. The server accepts and handles incoming requests */
	SERVER_STATE_RUNNING,
	/** State after calling freeHttpServer. In this state, the server just waits for the final connections to end and does not accept any new ones */
	SERVER_STATE_FREEING
} HttpServerState;

/**
 * Struct to represent an HTTP server
 */
typedef struct HttpServerStruct
{
	/** Represents the current state of the server */
	HttpServerState state;
	/** Stores how many connections to clients are currently open */
	unsigned long open_connections;
	/** Accepts new client connections */
	Socket *server_socket;
	/** Stores pairs of regular expressions and request handlers */
	GArray *handler_mappings;
} HttpServer;

/* Methods used for configuring and running servers */

/**
 * Creates an HTTP server on the specified port. The server does not accept any connections until
 * startHttpServer() is called. The caller takes responsibility for eventually calling
 * destoryHttpServer() on the returned pointer.
 *
 * @param port			the port to which the new server should be bound
 * @result				the created HTTP server
 */
API HttpServer *createHttpServer(char *port);

/**
 * Stops and tears down the HTTP server. Waits for accepted connections to disconnect (if any)
 * and frees all memory associated with the server (including registered request handlers).
 */
API void destroyHttpServer(HttpServer *server);

/**
 * Causes the server to start accepting connections.
 */
API bool startHttpServer(HttpServer *server);

/**
 * Causes the passed request handler to be called when an HttpRequest with a matching URI comes in.
 * In order to determine the matching precedence, matches are tested in the order in which they were
 * registered. Note that the caller retains ownership of all passed parameters (the regexp is copied).
 * It is *NOT* necessary to unregister every handler before decommissioning a server, all remaining
 * handlers are removed automatically.
 *
 * @param server				the server in question
 * @param hierarchical_regexp	the regular expression used to determine whether the request matches
 * @param handler				a handler function to be called for matching requests
 * @param userdata				custom userdata passed to the handler
 */
API void registerHttpServerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata);

/**
 * Removes a registered request handler. Does nothing if the parameters do not match any handler.
 *
 * @param server				the server in question
 * @param hierarchical_regexp	the regular expression passed to register the handler
 * @param handler				the handler function passed at registration time
 * @param userdata				the userdata passed at registration time
 */
API void unregisterHttpServerRequestHandler(HttpServer *server, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata);

/* Accessor methods for HttpRequest. Note that these are all read-only */

/**
 * Returns whether or not the request has a value associated with key.
 *
 * @param request			the request to check
 * @param key				the key to check
 * @result					true if such a parameter exists
 */
API bool checkHttpRequestParameter(HttpRequest *request, char *key);

/**
 * Returns the value associated with key if there is one, and NULL otherwise. The caller is responsible
 * for freeing the returned string.
 *
 * @param request			the request for which the parameter should be returned
 * @param key				the key of the parameter that should be returned
 * @result					the returned parameter of NULL on failure
 */
API char *getHttpRequestParameter(HttpRequest *request, char *key);

/* Mutator methods for HttpResponse */

/**
 * Adds content to an HTTP response object
 *
 * @param response			the HTTP response to add the content to
 * @param content			the printf-style content to append to the response
 */
API void appendHttpResponseContent(HttpResponse *response, char *content, ...);

/**
 * Resets the content of the response to the empty string
 *
 * @param response			the HTTP response to clear
 */
API void clearHttpResponseContent(HttpResponse *response);

#endif
