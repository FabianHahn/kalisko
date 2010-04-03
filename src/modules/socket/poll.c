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

#include <assert.h>
#include <glib.h>

#include "dll.h"
#include "hooks.h"
#include "timer.h"
#include "log.h"

#include "api.h"
#include "socket.h"
#include "poll.h"

static gboolean pollSocket(void *fd_p, void *socket_p, void *data);

static GHashTable *poll_table;
static char poll_buffer[SOCKET_POLL_BUFSIZE];
static int pollInterval;
static GTimeVal *lastScheduledPollTime;

TIMER_CALLBACK(poll);

/**
 * Initializes socket polling via hooks
 * @param interval		the polling interval to use
 */
API void initPoll(int interval)
{
	pollInterval = interval;

	HOOK_ADD(socket_read);
	HOOK_ADD(socket_error);
	HOOK_ADD(socket_disconnect);
	HOOK_ADD(socket_client_connected);

	poll_table = g_hash_table_new(&g_int_hash, &g_int_equal);

	lastScheduledPollTime = TIMER_ADD_TIMEOUT(pollInterval, poll);
}

/**
 * Frees socket polling via hooks
 */
API void freePoll()
{
	TIMER_DEL(lastScheduledPollTime);

	HOOK_DEL(socket_read);
	HOOK_DEL(socket_error);
	HOOK_DEL(socket_disconnect);
	HOOK_DEL(socket_client_connected);

	g_hash_table_destroy(poll_table);
}

/**
 * Enables polling for a socket
 * @param socket		the socket to enable the polling for
 * @result				true if successful
 */
API bool enableSocketPolling(Socket *socket)
{
	if(g_hash_table_lookup(poll_table, &socket->fd) != NULL) { // Socket with that fd is already polled
		return false;
	}

	g_hash_table_insert(poll_table, &socket->fd, socket);
	return true;
}

/**
 * Disables polling for a socket
 * @param socket		the socket to disable the polling for
 * @result				true if successful
 */
API bool disableSocketPolling(Socket *socket)
{
	return g_hash_table_remove(poll_table, &socket->fd) == TRUE;
}

/**
 * Poll all sockets signed up for polling
 */
API void pollSockets()
{
	g_hash_table_foreach_remove(poll_table, &pollSocket, NULL);
}

/**
 * Callback to poll all sockets signed up for polling
 */
TIMER_CALLBACK(poll)
{
	pollSockets();
	lastScheduledPollTime = TIMER_ADD_TIMEOUT(pollInterval, poll);
}

/**
 * A GHRFunc used to poll a socket
 * @param fd_p		a pointer to the socket's file descriptor
 * @param socket_p	the socket
 * @param data		unused
 */
static gboolean pollSocket(void *fd_p, void *socket_p, void *data)
{
	int ret;
	Socket *socket = socket_p;

	if(!socket->connected) { // Remove socket from poll list if disconnected
		LOG_WARNING("Polling not connected socket %d, removing from list", socket->fd);
		return TRUE;
	}

	if(!socket->server) {
		if((ret = socketReadRaw(socket, poll_buffer, SOCKET_POLL_BUFSIZE)) < 0) {
			if(!socket->connected) { // socket was disconnected
				HOOK_TRIGGER(socket_disconnect, socket);
			} else { // error
				HOOK_TRIGGER(socket_error, socket);
			}

			return TRUE;
		} else if(ret > 0) { // we actually read something
			HOOK_TRIGGER(socket_read, socket, poll_buffer, ret);
		}
	} else {
		Socket *clientSocket;
		bool serverSockStatus = serverSocketAccept(socket, &clientSocket);

		switch(serverSockStatus) {
			case SERVER_SOCKET_NEW_CONNECTION:
				HOOK_TRIGGER(socket_client_connected, clientSocket);
				break;
			case SERVER_SOCKET_ERROR:
				HOOK_TRIGGER(sock_error, socket);

				LOG_SYSTEM_ERROR("Server socket error occured on socket %d", socket->fd);
				break;
		}
	}

	return FALSE;
}
