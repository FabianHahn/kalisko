/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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
#define HTTPSERVER_HTTPVERVER_H

#include "modules/socket/socket.h"

#include <glib.h>

/* Forward declaration to allow HttpRequest to reference a server */
struct s_HttpServer;

/**
 * Enum to represent the types of http request which can come in
 */
typedef enum
{
  HTTP_REQUEST_METHOD_GET,
  HTTP_REQUEST_METHOD_POST
} HttpRequestMethod;

/**
 * Struct to represent an HTTP request
 */
typedef struct
{
  /** The server which accepted the client with this request */
  struct s_HttpServer *server;

  HttpRequestMethod method;

  /** Store the URL of the request without the trailing parameters */
  char *url;

  /** Stores the parameters of the request */
  GHashTable *parameters;

  /** Used as intermediate storage for incomplete lines coming out of the socket stream */
  GString *line_buffer;

  /** Stores whether or not an empty line has been seen (and thus the stream is expected to end). */
  bool parsing_complete;

  /** Stores whether or not the minimal necessary data required to respond to the request has been parsed successfully. */
  bool valid;
} HttpRequest;

/**
 * Struct to represent an HTTP response
 */
typedef struct
{
    int status_code;
  char *content;
} HttpResponse;

/** 
 * A type of function which can respond to Http requests by populating a response struct
 */
typedef bool (HttpRequestHandler) (HttpRequest *request, HttpResponse *response);

/**
 * Struct to represent an HTTP server
 */
typedef struct s_HttpServer
{
    /** Accepts new client connections */
    Socket *server_socket;

    /** Stores pairs of regular expressions and request handlers */
    GArray *handler_mappings;
} HttpServer;

API HttpServer *createHttpServer(char *port);
API void freeHttpServer(HttpServer *server);
API bool startHttpServer(HttpServer *server);
API void registerRequestHandler(HttpServer *server, char *url_regexp, HttpRequestHandler *handler);

#endif
