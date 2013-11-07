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
#include "modules/rpc/line_server.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/store/clone.h"
#define API

// TODO: Refactor HttpServer to be built on top of this.

static LineServer *createLineServer();
static void destroyLineServer(LineServer *server);
static void tryFreeServer(LineServer *server);

// Takes no ownership of the passed pointers.
static LineServerClient *createLineServerClient(LineServer *server, Socket *socket);
static void destroyLineServerClient(LineServerClient *client);

// Extract all completed lines from the client buffer and updates the lines array of the client.
// After a call to this function, the client buffer contains no more newlines. Returns the number
// of new lines added to client->lines.
static unsigned int processClientBuffer(LineServerClient *client);

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args);
static void clientSocketRead(void *subject, const char *event, void *data, va_list args);
static void clientAccepted(void *subject, const char *event, void *data, va_list args);

API LineServer *startLineServer(char *port, LineServerCallback callback)
{
	LineServer *server = createLineServer(port, callback);
	logInfo("Starting LineServer on port %s", port);
	attachEventListener(server->socket, "accept", server, &clientAccepted);
	if(!connectSocket(server->socket)) {
		logInfo("Unable to connect server socket on port %s", server->socket->port);
		return false;
	}
	enableSocketPolling(server->socket);
	server->state = RPC_SERVER_STATE_RUNNING;
	return server;
}

API void stopLineServer(LineServer *server)
{
	disableSocketPolling(server->socket);
	detachEventListener(server->socket, "accept", server, &clientAccepted);
	server->state = RPC_SERVER_STATE_STOPPED;

	// Causes the server to get freed as soon as there are no more open connections.
	tryFreeServer(server);
}

API void disconnectLineServerClient(LineServerClient *client)
{
	// TODO: Close the socket. See if clientSocketDisconnected is called.
}

API void sendToLineServerClient(LineServerClient *client, char *message)
{
	socketWriteRaw(client->socket, message, strlen(message) * sizeof(char));
}

// ***********************************************************
// ************ Memory management for LineServer *************
// ***********************************************************

LineServer *createLineServer(char *port, LineServerCallback callback)
{
	LineServer *result = ALLOCATE_OBJECT(LineServer);
	result->state = RPC_SERVER_STATE_STOPPED;
	result->open_connections = 0;
	result->socket = createServerSocket(port);
	result->callback = callback;
	return result;
}

static void destroyLineServer(LineServer *server)
{
	freeSocket(server->socket);
	free(server);
}

static void tryFreeServer(LineServer *server)
{
	if (server->state != RPC_SERVER_STATE_STOPPED) {
		logError("Illegal call to tryFreeServer in state other than STOPPED: %d", server->state);
		return;
	}
	if (server->open_connections == 0) {
		destroyLineServer(server);
	}
}

// *****************************************************************
// ************ Memory management for LineServerClient *************
// *****************************************************************

static LineServerClient *createLineServerClient(LineServer *server, Socket *socket)
{
	LineServerClient *result = ALLOCATE_OBJECT(LineServerClient);
	result->socket = socket;
	result->server = server;
	result->lines = g_ptr_array_new_with_free_func(&free);
	result->line_buffer = g_string_new("");
	return result;
}

static void destroyLineServerClient(LineServerClient *client)
{
	g_ptr_array_free(client->lines, true);     // Calls free on the stored pointers.
	g_string_free(client->line_buffer, true);  // Frees the underlying raw string.
	free(client);
}

// ****************************************************************
// ********************** Socket callbacks ************************
// ****************************************************************

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args)
{
	LineServerClient *client = data;
	LineServer *server = client->server;
	Socket *client_socket = subject;

	disableSocketPolling(client_socket);
	detachEventListener(client_socket, "read", server, &clientSocketRead);
	detachEventListener(client_socket, "disconnect", server, &clientSocketDisconnected);
	freeSocket(client_socket);
	
	destroyLineServerClient(client);

	server->open_connections--;
	if (server->state == RPC_SERVER_STATE_STOPPED) {
		tryFreeServer(server);
	}
}

static void clientSocketRead(void *subject, const char *event, void *data, va_list args)
{
	LineServerClient *client = data;
	char *message = va_arg(args, char *);
	logTrace("Read message: %s", message);

	g_string_append(client->line_buffer, message);
	unsigned int added_lines = processClientBuffer(client);
	if (added_lines > 0) {
		LineServerCallback callback = client->server->callback;
		callback(client);
	}
}

static void clientAccepted(void *subject, const char *event, void *data, va_list args)
{
	LineServer *server = data;
	server->open_connections++;

	Socket *s = va_arg(args, Socket *);
	LineServerClient *client = createLineServerClient(server, s);

	attachEventListener(s, "read", client, &clientSocketRead);
	attachEventListener(s, "disconnect", client, &clientSocketDisconnected);
	enableSocketPolling(s);
}

static unsigned int processClientBuffer(LineServerClient *client)
{
	// TODO: Implement this.
	return 1337;
}
