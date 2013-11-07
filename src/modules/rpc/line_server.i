/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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

#ifndef RPC_RPCSERVER_H
#define RPC_RPCSERVER_H

#include <glib.h>
#include <stdarg.h>
#include "modules/socket/socket.h"
#include "modules/store/store.h"

// Forward declaration.
struct LineServerStruct;

typedef struct
{
	/** Stores the socket on which to communicate with the client. */
	Socket *socket;
	/** Stores the line server instance this client is connected to. */
	struct LineServerStruct *server;
	/** Stores the lines (as char *) of raw input received from the client since they connected. */
	GPtrArray *lines;
	/** Stores the line currently being processed. Private, client should not touch this. */
	GString *line_buffer;
} LineServerClient;

/** Function executed by the server whenever a client send a new line. */
typedef void (*LineServerCallback) (LineServerClient*);

typedef enum
{
	/** State in which the rpc server accepts and processes new incoming connections. */
	RPC_SERVER_STATE_RUNNING,
	/** State of the rpc server after calling stop or between creation and start. */
	RPC_SERVER_STATE_STOPPED
} LineServerState;

/**
 * Struct representing the rpc server.
 */
typedef struct LineServerStruct
{
	/** The current state of the server. */
	LineServerState state;
	/** The server socket which accepts new connections. */
	Socket *socket;
	/** Stores the number of client currently connected. */
	unsigned long open_connections;
	/** Callback executed when a user makes an rpc all. */
	LineServerCallback callback;
} LineServer;

/**
 * Creates and starts the central rpc server.
 * @param   port           the port on which to listen
 * @param   lineCallback   a callback executed whenever a client has finished sending a full line
 * @return  a newly running LineServer. The caller is responsible for eventually calling stopLineServer()
 */
API LineServer *startLineServer(char *port, LineServerCallback callback);

/**
 * Stops the provided LineServer and frees it. Calling this invalidates the passed pointer.
 * @param   server       the server to stop
 */
API void stopLineServer(LineServer *server);

/**
 * Causes the server to disconnect from the client.
 * @param   client       the client to disconnect
 */
API void disconnectLineServerClient(LineServerClient *client);

/**
 * Send the passed string to the client in question.
 * @param   client       the client to talk to
 * @param   message      the string to send to the client
 */
API void sendToLineServerClient(LineServerClient *client, char *message);

#endif
