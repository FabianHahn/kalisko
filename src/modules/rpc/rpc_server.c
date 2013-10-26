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
#include "modules/rpc/rpc_server.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/store/clone.h"
#define API

// TODO: Change this to be a generic line-by-line-read server, move this to an own module and
// refactor HttpServer to be implemented on top of this as well.

static RpcServer *createRpcServer();
static void destroyRpcServer(RpcServer *server);
static void tryFreeServer(RpcServer *server);

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args);
static void clientSocketRead(void *subject, const char *event, void *data, va_list args);
static void clientAccepted(void *subject, const char *event, void *data, va_list args);

API RpcServer *startRpcServer(char *port, RpcCallback rpc_callback)
{
	RpcServer *server = createRpcServer(port, rpc_callback);
	logInfo("Starting RpcServer on port %s", port);
	attachEventListener(server->server_socket, "accept", server, &clientAccepted);
	if(!connectSocket(server->server_socket)) {
		logInfo("Unable to connect server socket on port %s", server->server_socket->port);
		return false;
	}
	enableSocketPolling(server->server_socket);
	return server;
}

API void stopRpcServer(RpcServer *server)
{
	// TODO: Make sure the server no longer accepts new connections by setting state to stopped.
	disableSocketPolling(server->server_socket);
	detachEventListener(server->server_socket, "accept", server, &clientAccepted);

	// Causes the server to get freed as soon as there are no more open connections.
	tryFreeServer(server);
}

RpcServer *createRpcServer(char *port, RpcCallback *rpc_callback)
{
	RpcServer *result = ALLOCATE_OBJECT(RpcServer);
	result->open_connections = 0;
	result->server_socket = createServerSocket(port);
	result->rpc_callback = rpc_callback;
	return result;
}

static void destroyRpcServer(RpcServer *server)
{
	freeSocket(server->server_socket);
	free(server);
}

static void tryFreeServer(RpcServer *server)
{
	if (server->open_connections == 0) {
		destroyRpcServer(server);
	}
}

static void clientSocketDisconnected(void *subject, const char *event, void *data, va_list args)
{
	RpcServer *server = data;
	Socket *client_socket = subject;

	disableSocketPolling(client_socket);
	detachEventListener(client_socket, "read", server, &clientSocketRead);
	detachEventListener(client_socket, "disconnect", server, &clientSocketDisconnected);
	freeSocket(client_socket);

	server->open_connections--;
	// TODO: Call tryFreeServer if the server is in stopped state.
}

static void clientSocketRead(void *subject, const char *event, void *data, va_list args)
{
	// TODO: Implement line-by-line reading instead of just mirroring what is read.

	Socket *client_socket = subject;
	char *message = va_arg(args, char *);
	logTrace("Read message: %s", message);
	socketWriteRaw(client_socket, message, strlen(message) * sizeof(char));
}

static void clientAccepted(void *subject, const char *event, void *data, va_list args)
{
	RpcServer *server = data;
	server->open_connections++;

	Socket *s = va_arg(args, Socket *);
	attachEventListener(s, "read", server, &clientSocketRead);
	attachEventListener(s, "disconnect", server, &clientSocketDisconnected);
	enableSocketPolling(s);
}
