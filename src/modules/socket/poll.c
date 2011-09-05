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
#include "timer.h"
#include "log.h"
#include "memory_alloc.h"
#include "modules/event/event.h"

#define API
#include "socket.h"
#include "poll.h"

static bool pollSocket(Socket *socket, int *fd_p);

static GHashTable *poll_table;
static char poll_buffer[SOCKET_POLL_BUFSIZE];
static int pollInterval;

/**
 * True if we're currently polling
 */
static bool polling;

TIMER_CALLBACK(poll);

/**
 * Initializes socket polling via hooks
 * @param interval		the polling interval to use
 */
API void initPoll(int interval)
{
	pollInterval = interval;

	poll_table = g_hash_table_new_full(&g_int_hash, &g_int_equal, &free, NULL);

	TIMER_ADD_TIMEOUT(pollInterval, poll);

	polling = false;
}

/**
 * Frees socket polling via hooks
 */
API void freePoll()
{
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

	int *fd = ALLOCATE_OBJECT(int);
	*fd = socket->fd;

	g_hash_table_insert(poll_table, fd, socket);
	return true;
}

/**
 * Disables polling for a socket
 * @param socket		the socket to disable the polling for
 * @result				true if successful
 */
API bool disableSocketPolling(Socket *socket)
{
	return g_hash_table_remove(poll_table, &socket->fd) == true;
}

/**
 * Poll all sockets signed up for polling if not currently polling already
 */
API void pollSockets()
{
	if(!polling) {
		polling = true; // set polling flag to lock our poll table
		GList *sockets = g_hash_table_get_values(poll_table); // get a static list of sockets so we may modify the hash table while polling
		for(GList *iter = sockets; iter != NULL; iter = iter->next) {
			Socket *poll = iter->data;
			int fd; // storage for the file descriptor that won't be available anymore in case the socket gets freed before we remove it
			if(pollSocket(poll, &fd)) { // poll the socket
				// The socket should no longer be polled
				g_hash_table_remove(poll_table, &fd); // remove it from the polling table
			}
		}
		g_list_free(sockets); // remove our temporary iteration list
		polling = false; // release pseudo lock on poll table
	}
}

/**
 * Checks whether sockets are currently being polled
 */
API bool isSocketsPolling()
{
	return polling;
}

/**
 * Retrieves a socket for which polling is enabled by its file descriptor
 *
 * @param fd	the fd to lookup
 * @result		the socket or NULL if no socket with this fd is being polled
 */
API Socket *getPolledSocketByFd(int fd)
{
	return g_hash_table_lookup(poll_table, &fd);
}

/**
 * Callback to poll all sockets signed up for polling
 */
TIMER_CALLBACK(poll)
{
	pollSockets();
	$(int, event, triggerEvent)(NULL, "sockets_polled");
	TIMER_ADD_TIMEOUT(pollInterval, poll);
}

/**
 * Polls a socket and notifies the caller of whether it should be removed from the polling table afterwards
 *
 * @param socket	the socket to poll
 * @param fd_p		a pointer to an integer field to which the file descriptor of the socket should be written in case the socket should be removed from the polling table and could already be freed at that time
 * @result			true if the socket should be removed from the polling table after polling
 */
static bool pollSocket(Socket *socket, int *fd_p)
{
	*fd_p = socket->fd; // backup file descriptor

	if(!socket->connected) { // Socket is disconnected
		$(int, event, triggerEvent)(socket, "disconnect");
		return true;
	}

	if(socket->type != SOCKET_SERVER && socket->type != SOCKET_SERVER_BLOCK) {
		int ret;
		if((ret = socketReadRaw(socket, poll_buffer, SOCKET_POLL_BUFSIZE)) < 0) {
			if(!socket->connected) { // socket was disconnected
				$(int, event, triggerEvent)(socket, "disconnect");
			} else { // error
				$(int, event, triggerEvent)(socket, "error");
			}

			return true;
		} else if(ret > 0) { // we actually read something
			$(int, event, triggerEvent)(socket, "read", poll_buffer, ret);
		}
	} else {
		Socket *clientSocket;

		if((clientSocket = socketAccept(socket)) != NULL) {
			$(int, event, triggerEvent)(socket, "accept", clientSocket);
		} else {
			$(int, event, triggerEvent)(socket, "error");
		}
	}

	return false;
}
