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

#ifndef SOCKET_POLL_H
#define SOCKET_POLL_H

#include "socket.h"


/**
 * Initializes socket polling via hooks
 * @param interval		the polling interval to use
 */
API void initPoll(int interval);

/**
 * Frees socket polling via hooks
 */
API void freePoll();


/**
 * Asynchronously connects a client socket. Instead of waiting for the socket to be connected, this function does not block and returns immediately.
 * As soon as the socket is connected, it will trigger the "connected" event and will start polling automatically (this might even happen before this
 * function returns!). If connecting fails due to errors or timeout, the "disconnect" event will be triggered and you can either recall this function
 * or free the socket.
 *
 * @param s			the client socket to connect
 * @param timeout	time in seconds after which the connection should timeout and the "timeout" event should be triggered
 * @result			true if successful
 */
API bool connectClientSocketAsync(Socket *s, int timeout);

/**
 * Enables polling for a socket
 *
 * @param socket		the socket to enable the polling for
 * @result				true if successful
 */
API bool enableSocketPolling(Socket *socket);

/**
 * Checks whether polling is enabled for a certain socket
 *
 * @param socket		the socket for which to check whether socket polling is enabled
 * @result				true if socket polling is enabled for the socket
 */
API bool isSocketPollingEnabled(Socket *socket);

/**
 * Disables polling for a socket
 * @param socket		the socket to disable the polling for
 * @result				true if successful
 */
API bool disableSocketPolling(Socket *socket);

/**
 * Poll all sockets signed up for polling if not currently polling already
 */
API void pollSockets();

/**
 * Checks whether sockets are currently being polled
 *
 * @result		true if sockets are currently being polled
 */
API bool isSocketsPolling();

/**
 * Retrieves a socket for which polling is enabled by its file descriptor
 *
 * @param fd	the fd to lookup
 * @result		the socket or NULL if no socket with this fd is being polled
 */
API Socket *getPolledSocketByFd(int fd);

/**
 * Checks whether a socket is still active, i.e. that it is either connected or that it is disconnected but socket polling is still enabled
 *
 * @param socket		the socket that should be checked for being still active
 * @result				true if the specified socket is still active
 */
static inline bool isSocketActive(Socket *socket)
{
	return socket->connected || isSocketPollingEnabled(socket);
}

#define SOCKET_POLL_BUFSIZE 4096

#endif
