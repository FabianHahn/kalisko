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

#ifdef WIN32
#include <stdio.h> // _fdopen
#include <fcntl.h> // _open_osfhandle
#include <winsock2.h> // recv, send, getaddrinfo, socket, connect, select, timevalr
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <ws2tcpip.h> // getaddrinfo, addrinfo, freeaddrinfo
#else
#include <sys/socket.h> // recv, send, getaddrinfo, socket, connect
#include <netdb.h> // getaddrinfo, addrinfo, freeaddrinfo
#include <fcntl.h>
#include <sys/select.h> // select, timeval
#include <netinet/in.h> // FreeBSD needs this, linux doesn't care ;)
#endif
#include <glib.h> // GList
#include <stdlib.h> // malloc, free
#include <unistd.h> // close
#include <sys/types.h> // recv, send, getaddrinfo, socket, connect
#include <errno.h> // errno, EINTR
#include <assert.h>

#include "dll.h"
#include "timer.h"
#include "log.h"
#include "memory_alloc.h"
#include "modules/event/event.h"

#define API
#include "socket.h"
#include "poll.h"
#include "util.h"

static bool pollConnectingSocket(Socket *socket);
static bool pollSocket(Socket *socket, int *fd_p);

static GHashTable *poll_table;
static char poll_buffer[SOCKET_POLL_BUFSIZE];
static int pollInterval;

/**
 * True if we're currently polling
 */
static bool polling;

/**
 * List of currently connecting sockets
 */
static GQueue *connecting;

TIMER_CALLBACK(poll);

/**
 * Initializes socket polling via hooks
 * @param interval		the polling interval to use
 */
API void initPoll(int interval)
{
	pollInterval = interval;

	poll_table = g_hash_table_new_full(&g_int_hash, &g_int_equal, &free, NULL);
	connecting = g_queue_new();

	TIMER_ADD_TIMEOUT(pollInterval, poll);

	polling = false;
}

/**
 * Frees socket polling via hooks
 */
API void freePoll()
{
	g_hash_table_destroy(poll_table);
	g_queue_free(connecting);
}

/**
 * Asynchronously connects a client socket. Instead of waiting for the socket to be connected, this function does not block and returns immediately.
 * As soon as the socket is connected, it will trigger the "connected" event and will start polling automatically (this might even happen before this
 * function returns!). If connecting faills, the "error" event will be triggered and you can either recall this function or free the socket.
 *
 * @param s			the client socket to connect
 * @result			true if successful
 */
API bool connectClientSocketAsync(Socket *s)
{
	if(s->connected) {
		LOG_ERROR("Cannot connect already connected socket %d", s->fd);
		return false;
	}

	if(s->type != SOCKET_CLIENT) {
		LOG_ERROR("Cannot asynchronously connect non-client socket");
		return false;
	}

	struct addrinfo hints;
	struct addrinfo *server;
	int ret;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; // don't care if we use IPv4 or IPv6 to reach our destination
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	if((ret = getaddrinfo(s->host, s->port, &hints, &server)) != 0) {
		LOG_ERROR("Failed to look up address %s:%s: %s", s->host, s->port, gai_strerror(ret));
		return false;
	}

	if((s->fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) == -1) {
		LOG_SYSTEM_ERROR("Failed to create socket");
		return false;
	}

	if(!setSocketNonBlocking(s->fd)) {
		LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
		return false;
	}

	LOG_INFO("Asynchronously connecting client socket %d to %s:%s...", s->fd, s->host, s->port);

	if(connect(s->fd, server->ai_addr, server->ai_addrlen) < 0) { // try to connect socket
#ifdef WIN32
		if(WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK) {
#else
		if(errno == EINPROGRESS) {
#endif
			g_queue_push_tail(connecting, s); // add to connecting list
			LOG_DEBUG("Socket %d delayed connection, queueing...", s->fd);
		} else {
#ifdef WIN32
			char *error = g_win32_error_message(WSAGetLastError());
			LOG_ERROR("Connection for socket %d failed: %s", s->fd, error);
			free(error);
#else
			LOG_SYSTEM_ERROR("Connection for socket %d failed", s->fd);
#endif
			freeaddrinfo(server);
			return false;
		}
	} else {
		LOG_INFO("Direct response for asynchronous connection on socket %d", s->fd);
		s->connected = true;
		triggerEvent(s, "connected");
	}

	freeaddrinfo(server);
	return true;
}

/**
 * Enables polling for a socket
 *
 * @param socket		the socket to enable the polling for
 * @result				true if successful
 */
API bool enableSocketPolling(Socket *socket)
{
	if(isSocketPollingEnabled(socket)) { // Socket with that fd is already polled
		return false;
	}

	int *fd = ALLOCATE_OBJECT(int);
	*fd = socket->fd;

	g_hash_table_insert(poll_table, fd, socket);
	return true;
}

/**
 * Checks whether polling is enabled for a certain socket
 *
 * @param socket		the socket for which to check whether socket polling is enabled
 * @result				true if socket polling is enabled for the socket
 */
API bool isSocketPollingEnabled(Socket *socket)
{
	return g_hash_table_lookup(poll_table, &socket->fd) != NULL;
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
		polling = true; // set polling flag to lock our poll table in order to make this function reentrancy safe

		if(!g_queue_is_empty(connecting)) {
			GQueue *connectingSockets = g_queue_copy(connecting); // copy the connecting socket list so we may modify the list while polling
			for(GList *iter = connectingSockets->head; iter != NULL; iter = iter->next) {
				if(pollConnectingSocket(iter->data)) { // poll the connecting socket
					// The socket should no longer be polled
					g_queue_remove(connecting, iter->data); // remove it from the original connecting queue
				}
			}
			g_queue_free(connectingSockets); // remove our temporary iteration list
		}

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
 *
 * @result		true if sockets are currently being polled
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
 * Polls a connecting socket and notifies the caller of whether it should be removed from the connecting polling queue afterwards
 *
 * @param socket		the connecting socket to poll
 * @result				true if the socket should be removed from the connecting polling queue after polling
 */
static bool pollConnectingSocket(Socket *socket)
{
	assert(!socket->connected);
	assert(socket->type == SOCKET_CLIENT);

	// Initialize timeout
	struct timeval tv = {0, 0};

	// Initialize write fd set
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(socket->fd, &fdset);

	// Select socket for write flag (connected)
	if(select(socket->fd + 1, NULL, &fdset, NULL, &tv) < 0) {
#ifdef WIN32
		if(WSAGetLastError() != WSAEINTR) {
			char *error = g_win32_error_message(WSAGetLastError());
			LOG_ERROR("Error selecting socket %d for write flag (connected) while polling: %s", socket->fd, error);
			free(error);
#else
		if(errno != EINTR) {
			LOG_SYSTEM_ERROR("Error selecting socket %d for write flag (connected) while polling", socket->fd);
#endif
			triggerEvent(socket, "error");
			return true;
		}

		// EINTR at this point means the socket is just not connected yet, so we can safely return and continue polling another time
		return false;
	} else { // there is a write flag on the socket
		// Socket selected for write, check if we're indeed connected
		int valopt;
		socklen_t lon = sizeof(int);
		if(getsockopt(socket->fd, SOL_SOCKET, SO_ERROR, (void*) (&valopt), &lon) < 0) {
			LOG_SYSTEM_ERROR("getsockopt() failed on socket %d", socket->fd);
			triggerEvent(socket, "error");
			return true;
		} else if(valopt != 0) { // There was a connection error
			LOG_SYSTEM_ERROR("Asynchronous connection on socket %d failed", socket->fd);
			triggerEvent(socket, "error");
			return true;
		}

		LOG_INFO("Asynchronously connected socket %d", socket->fd);
		socket->connected = true;
		triggerEvent(socket, "connected");
		return true;
	}
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
