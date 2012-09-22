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

#define FILE_NOT_FOUND_STATUS_CODE 404
#define BAD_REQUEST_STATUS_CODE 501

MODULE_NAME("httpserver");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server library which can be used to easily create http servers.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

/** Maps a socket to an HttpRequest struct which is currently under construction because the stream is being read */
static GHashTable *pending_requests;

static void listener_readRequest(void *subject, const char *event, void *data, va_list args);
static void listener_serverSocketAccept(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
  pending_requests = g_hash_table_new(g_int_hash, g_int_equal);  
  return true;
}

MODULE_FINALIZE
{
  g_hash_table_destroy(pending_requests);
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
  server->state = SERVER_STATE_CREATED;
  server->open_connections = 0;
  server->server_socket = createServerSocket(port); 
  server->handler_mappings = g_array_new(FALSE, FALSE, sizeof(RequestHandlerMapping*));	

  enableSocketPolling(server->server_socket);
  attachEventListener(server->server_socket, "accept", server, &listener_serverSocketAccept);

  return server;
}

static void tryFreeServer(HttpServer *server)
{
	if(server->open_connections == 0) {
	  LOG_DEBUG("Freeing HTTP server resources");

	  // Clean up all the created handler mappings
	  GArray *mappings = server->handler_mappings;
	  for (int i = 0; i < mappings->len; ++i) {
		RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
		free(mapping->regexp);
		free(mapping);
	  }
	  free(mappings);

	  // Finally, clean up the server struct itself
	  free(server);
	}
}

/**
 * Stops and tears down the internals of an HTTP server and releases the associated memory. Also frees the server struct itself.
 * TODO: rename this to stopHttpServer which guarantees that memory will be freed eventually
 */
API void freeHttpServer(HttpServer *server)
{
  LOG_DEBUG("Freeing HttpServer on port %s", server->server_socket->port);

  // Clean up the server socket
  disconnectSocket(server->server_socket);
  detachEventListener(server->server_socket, "accept", NULL, &listener_serverSocketAccept);
  disableSocketPolling(server->server_socket);
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
  enableSocketPolling(s);

  HttpRequest *request = ALLOCATE_OBJECT(HttpRequest);
  request->parsing_complete = false;
  request->valid = false;
  request->line_buffer = g_string_new("");
  request->server = data;
  request->parameters = g_hash_table_new_full(g_int_hash, g_int_equal, &free, &free);  

  request->server->open_connections++;
	LOG_DEBUG("Server connections: %lu", request->server->open_connections);

  g_hash_table_insert(pending_requests, s, request);  // TODO: figure out whether this call need cleanup
  attachEventListener(s, "read", NULL, &listener_readRequest);
}

static bool parseMethod(HttpRequest *request, char *method)
{
  if(strcmp("GET", method) == 0) {
    request->method = HTTP_REQUEST_METHOD_GET;
    LOG_DEBUG("Request method is GET");
    return true;
  }
  if(strcmp("POST", method) == 0) {
    request->method = HTTP_REQUEST_METHOD_POST;
    LOG_DEBUG("Request method is POST");
    return true;
  }
  return false;
}

// Parses a single parameter from a string of the form "key=value". Returns whether or not parsing was successful.
static bool parseParameter(HttpRequest *request, char *keyvalue)
{
  char **parts = g_strsplit(keyvalue, "=", 0);
  int count = 0;
  bool success = true;
  for(char **iter = parts; *iter != NULL; ++iter) {
    ++count;
  }

  GHashTable *params = request->parameters;
  if(count == 2) {
    char *unescaped_key = g_uri_unescape_string(parts[0], NULL);
    char *unescaped_value = g_uri_unescape_string(parts[1], NULL);
    if(unescaped_key == NULL || unescaped_value == NULL) {
      LOG_DEBUG("Failed to unescape %s: %s", parts[0], parts[1]);
      success = false;
    } else {
      // TODO: handle the case where the key already has a value (using g_hash_table_replace)
      g_hash_table_insert(params, unescaped_key, unescaped_value);
      success = true;
    }
  } else {
    success = false;
  }

  g_strfreev(parts);
  return success;
}

// Parses parameters from a string of the form "key1=value1&key2=value2". Return whether or not parsing was successful.
static bool parseParameters(HttpRequest *request, char *query_part)
{
  char **parts = g_strsplit(query_part, "&", 0);
  bool success = true;
  for(char **iter = parts; *iter != NULL; ++iter) {
    success &= parseParameter(request, *iter);
  }
  g_strfreev(parts);
  return success;
}

static bool parseUrl(HttpRequest *request, char *url)
{
  LOG_DEBUG("Request URL is %s", url);
  bool success = true;

  if(strstr(url, "?") != NULL) {
    // Extract parameters
    char **parts = g_strsplit(url, "?", 0);
    int count = 0;
    for(char **iter = parts; *iter != NULL; ++iter) {
      ++count;
    }
    
    if(count == 2) {
      // Unescape the path part (which is the part before the ?)
      // TODO: possibly disallow special characters not allowed as file names (replace second param)
      request->url = g_uri_unescape_string(parts[0], NULL);

      // Parse the parameter part
      // TODO: handle a fragment part
      success &= parseParameters(request, parts[1]);
    } else {
      success = false;
    }
    g_strfreev(parts);
  } else {
    // No parameters
    request->url = g_uri_unescape_string(url, NULL);
  }
  return success;
}

/** Parses one line as an HTTP request. Can handle empty lines. */
static void parseLine(HttpRequest *request, char *line)
{
	// LOG_DEBUG("Parsing HTTP line: %s", line);
	if(strlen(line) == 0) {
		request->parsing_complete = true;
		return;
	}

  // For now, only detect lines of the form <METHOD> <URL> HTTP/<NUMBER>. Parsing one of these makes the request "valid"
  // TODO: extract parsing logic into an own file
  GRegex *regexp = g_regex_new("^(GET|POST)[ ]+(.+)[ ]+HTTP/\\d\\.\\d$", 0, 0, NULL);
  GMatchInfo *match_info;

  // TODO: figure out what happens exactly if the request is already valid
  if(g_regex_match(regexp, line, 0, &match_info)) {
    gchar *method = g_match_info_fetch(match_info, 1);
    gchar *url = g_match_info_fetch(match_info, 2);

    if(parseMethod(request, method) && parseUrl(request, url)) {
      request->valid = true;
    }

    free(method);
    free(url);
  }

  g_match_info_free(match_info);
  g_regex_unref(regexp);
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
  if(!request->valid) {
    sendStatusResponse(BAD_REQUEST_STATUS_CODE, client);
	  return;
  }

	// Go through all handlers and execute the first one which matches the requested URL
	HttpServer *server = request->server;
	GArray *mappings = server->handler_mappings;
	for (int i = 0; i < mappings->len; ++i) {
	  RequestHandlerMapping *mapping = g_array_index(mappings, RequestHandlerMapping*, i);
	  if(g_regex_match_simple(mapping->regexp, request->url, 0, 0)) {
      HttpResponse response;
      mapping->handler(request, &response);
      sendResponse(&response, client);
      return;
	  }
	}

	// If we got this far, there is no handler registered for this request
	sendStatusResponse(FILE_NOT_FOUND_STATUS_CODE, client);
}

static void listener_readRequest(void *subject, const char *event, void *data, va_list args)
{
  char *message = va_arg(args, char *);
  HttpRequest *request = g_hash_table_lookup(pending_requests, subject);
  if (request == NULL) {
    LOG_DEBUG("Read from socket without a mapped HttpRequest struct. Ignoring...");
    return;
  }

  g_string_append(request->line_buffer, message);
  checkForNewLine(request);

  if(request->parsing_complete) {
    handleRequest(request, subject);
    // TODO: Disconnect and clean up the client socket and detach handlers etc
	HttpServer *server = request->server;
	server->open_connections--;
	if(server->state == SERVER_STATE_FREEING) {
		tryFreeServer(server);
	}
  }
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
    GString *copy = g_string_new(original);
    return g_string_free(copy, true);
  }
}

/** Returns a pointer to the GHashTable representing the parameters (for advanced use such as iterating over the key-value pairs). It is the responsibility of the caller not to change the state of the GHashTable. The caller must not free the returned pointer. */
API GHashTable *getParameters(HttpRequest *request)
{
  return request->parameters;
}

