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

API Socket *createClientSocket(char *host, char *port);
API Socket *createServerSocket(char *port);
API Socket *createShellSocket(char **args);
API bool connectSocket(Socket *s);
API bool disconnectSocket(Socket *s);
API void freeSocket(Socket *s);
API bool socketWriteRaw(Socket *s, void *buffer, int size);
API int socketReadRaw(Socket *s, void *buffer, int size);
API Socket *socketAccept(Socket *server);

#endif
