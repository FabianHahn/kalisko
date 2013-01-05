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


#ifndef SOCKET_SOCKET_H
#define SOCKET_SOCKET_H

#ifdef WIN32
#include <stdio.h>
#endif

#include "types.h"

typedef enum {
	/** the socket is a client socket */
	SOCKET_CLIENT,
	/** the socket is a server socket */
	SOCKET_SERVER,
	/** the socket is a blocking server socket - do not use this unless you really know what you're doing! */
	SOCKET_SERVER_BLOCK,
	/** the socket is a client to one of our server sockets */
	SOCKET_SERVER_CLIENT,
	/** the socket is a shell socket */
	SOCKET_SHELL
} SocketType;

typedef struct {
	/** the file descriptor of the socket */
	int fd;
	char *host;
	char *port;
	SocketType type;
	bool connected;
	void *custom;
#ifdef WIN32
	FILE *out;
	FILE *in;
#endif
} Socket;


/**
 * Create a client socket
 *
 * @param host		the host to connect to
 * @param port		the port to connect to
 * @result			the created socket
 */
API Socket *createClientSocket(char *host, char *port);

/**
 * Create a server socket
 *
 * @param port		the port for server connections
 * @result			the created socket
 */
API Socket *createServerSocket(char *port);

/**
 * Create a shell socket
 *
 * @param args			the shell command arguments, terminated by NULL
 * @result				the created socket
 */
API Socket *createShellSocket(char **args);

/**
 * Connects a socket
 *
 * @param s			the socket to connect
 * @result			true if successful, false on error
 */
API bool connectSocket(Socket *s);

/**
 * Disconnects a socket. Call this function to get rid of a socket inside a socket_read hook, then free it inside a socket_disconnect listener.
 * See the documentation of freeSocket for further details on this issue.
 * @see freeSocket
 *
 * @param s			the socket to disconnect
 * @result			true if successful, false on error
 */
API bool disconnectSocket(Socket *s);

/**
 * Frees a socket. Note that this function MUST NOT be called from a (descendent of a) socket_read hook since further listeners expect the socket
 * to still be existing. If you want to get rid of a socket after a read event, listen to the socket_disconnect hook and disconnect it with disconnectSocket().
 * Then, free it inside the socket_disconnect hook using this function. If you don't want to adhere to this rule, you might as well shoot yourself in the foot.
 * @see disconnectSocket
 *
 * @param s			the socket to free
 */
API void freeSocket(Socket *s);

/**
 * Writes directly into a socket
 *
 * @param s				the socket to write to
 * @param buffer		the buffer to send
 * @param size			the buffer's size
 * @result				true if successful, false on error
 */
API bool socketWriteRaw(Socket *s, void *buffer, int size);

/**
 * Reads directly from a socket
 *
 * @param s				the socket to read from
 * @param buffer		the buffer to read into
 * @param size			the buffer's size
 * @result				number of bytes read, -1 on error
 */
API int socketReadRaw(Socket *s, void *buffer, int size);

/**
 * Accepts a client socket from a listening server socket
 *
 * @param server		the server socket
 * @return Socket		the accepted socket
 */
API Socket *socketAccept(Socket *server);

#endif
