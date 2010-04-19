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
#include <winsock2.h> // recv, send, getaddrinfo, socket, connect
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <ws2tcpip.h> // getaddrinfo, addrinfo, freeaddrinfo
#else
#include <sys/socket.h> // recv, send, getaddrinfo, socket, connect
#include <netdb.h> // getaddrinfo, addrinfo, freeaddrinfo
#include <fcntl.h>
#endif
#include <glib.h> // GList
#include <stdlib.h> // malloc, free
#include <unistd.h> // close
#include <sys/types.h> // recv, send, getaddrinfo, socket, connect
#include <errno.h> // errno, EINTR
#include <assert.h> // assert
#include <string.h> // strerror, strdup

#include "dll.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"
#include "module.h"
#include "hooks.h"
#include "modules/config/config.h"

#include "api.h"
#include "socket.h"
#include "poll.h"

MODULE_NAME("socket");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The socket module provides an API to establish network connections and transfer data over them");
MODULE_VERSION(0, 2, 3);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 2, 0));

MODULE_INIT
{
#ifdef WIN32
	WSADATA wsaData;

	if(WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
		LOG_ERROR("WSAStartup failed");
		return false;
	}
#endif

	int pollInterval = 100000;

	Store *configPollInterval = $(Store *, config, getConfigPathValue)("socket/pollInterval");
	if(configPollInterval != NULL && configPollInterval->type == STORE_INTEGER) {
		pollInterval = configPollInterval->content.integer;
	} else {
		LOG_WARNING("Could not determine config value socket/pollInterval, using default");
	}

	initPoll(pollInterval);
	return true;
}

MODULE_FINALIZE
{
#ifdef WIN32
	WSACleanup();
#endif

	freePoll();
}

/**
 * Create a client socket
 *
 * @param host		the host to connect to
 * @param buffer	the port to connect to
 * @result			the created socket
 */
API Socket *createClientSocket(char *host, char *port)
{
	Socket *s = ALLOCATE_OBJECT(Socket);

	s->fd = -1;
	s->host = strdup(host);
	s->port = strdup(port);
	s->server = false;
	s->connected = false;

	return s;
}

/**
 * Create a server socket
 *
 * @param port		the port for server connections
 * @result			the created socket
 */
API Socket *createServerSocket(char *port)
{
	Socket *s = ALLOCATE_OBJECT(Socket);

	s->fd = -1;
	s->host = NULL;
	s->port = strdup(port);
	s->server = true;
	s->connected = false;

	return s;
}

/**
 * Connects a socket
 *
 * @param s			the socket to connect
 * @result			true if successful, false on error
 */
API bool connectSocket(Socket *s)
{
	if(s->connected) {
		LOG_ERROR("Cannot connect already connected socket %d", s->fd);
		return false;
	}

	if(s->server) {
		struct addrinfo hints;
		struct addrinfo *server;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // fill in the IP automaticly

		int ret;
		if((ret = getaddrinfo(NULL, s->port, &hints, &server)) == -1) {
			LOG_ERROR("Failed to create address info for server socket");
			return false;
		}

		if((s->fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) == -1) {
			LOG_SYSTEM_ERROR("Failed to create server socket");
			return false;
		}

		s->connected = true;

		int param = 1;
		if(setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&param, sizeof(int)) == -1) {
			LOG_ERROR("Failed to set SO_REUSEADDR for socket %d", s->fd);
			freeSocket(s);

			return false;
		}

		if(bind(s->fd, server->ai_addr, server->ai_addrlen) == -1) {
			LOG_SYSTEM_ERROR("Failed to bind server socket %d", s->fd);
			freeSocket(s);

			return false;
		}

		if(listen(s->fd, 5) == -1) {
			LOG_SYSTEM_ERROR("Failed to listen server socket %d", s->fd);
			freeSocket(s);

			return false;
		}

		if(!setSocketNonBlocking(s->fd)) {
			LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
			freeSocket(s);

			return false;
		}

		freeaddrinfo(server);

	} else {
		struct addrinfo hints;
		struct addrinfo *server;

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC; // don't care if we use IPv4 or IPv6 to reach our destination
		hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

		int ret;

		if((ret = getaddrinfo(s->host, s->port, &hints, &server)) != 0) {
			LOG_ERROR("Failed to look up address %s:%s: %s", s->host, s->port, gai_strerror(ret));
			return false;
		}

		if((s->fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) == -1) {
			LOG_SYSTEM_ERROR("Failed to create socket");
			return false;
		}

		s->connected = true;

		if(connect(s->fd, server->ai_addr, server->ai_addrlen) != 0) {
			LOG_SYSTEM_ERROR("Failed to connect socket %d", s->fd);
			freeSocket(s);

			return false;
		}

		if(!setSocketNonBlocking(s->fd)) {
			LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
			freeSocket(s);

			return false;
		}

		freeaddrinfo(server);
	}

	return true;
}

/**
 * Disconnects a socket
 *
 * @param s			the socket to disconnect
 * @result			true if successful, false on error
 */
API bool disconnectSocket(Socket *s)
{
	if(s->connected) {
#ifdef WIN32
		if(closesocket(s->fd) != 0) {
#else
		if(close(s->fd) != 0) {
#endif
			LOG_SYSTEM_ERROR("Failed to close socket %d", s->fd);
			return false;
		}

		s->connected = false;

		return true;
	} else {
		LOG_ERROR("Cannot disconnect already disconnected socket");
		return false;
	}
}

/**
 * Frees a socket
 *
 * @param s			the socket to free
 * @result			true if successful, false on error
 */
API bool freeSocket(Socket *s)
{
	if(s->connected) {
		if(!disconnectSocket(s)) {
			return false;
		}
	}

	if(s->host) {
		free(s->host);
	}

	if(s->port) {
		free(s->port);
	}

	free(s);

	return true;
}

/**
 * Writes directly into a socket
 *
 * @param s				the socket to write to
 * @param buffer		the buffer to send
 * @param size			the buffer's size
 * @result				true if successful, false on error
 */
API bool socketWriteRaw(Socket *s, void *buffer, int size)
{
	int left = size;
	int ret;

	assert(size >= 0);

	if(!s->connected) {
		LOG_ERROR("Cannot write to disconnected socket");
		return false;
	}

	if(s->server) {
		LOG_ERROR("Cannot write to server socket");
		return false;
	}

	while(left > 0) {
		if((ret = send(s->fd, buffer, left, 0)) > 0) { // wrote ret characters
			left -= ret;
			buffer += ret;
		} else if(errno == EINTR) { // interrupted
			continue;
		} else { // error
			LOG_SYSTEM_ERROR("Failed to write to socket %d", s->fd);
			return false;
		}
	}

	return true;
}

/**
 * Reads directly from a socket
 *
 * @param s				the socket to read from
 * @param buffer		the buffer to read into
 * @param size			the buffer's size
 * @result				number of bytes read, -1 on error
 */
API int socketReadRaw(Socket *s, void *buffer, int size)
{
	int ret;

	assert(size >= 0);

	if(!s->connected) {
		LOG_ERROR("Cannot read from disconnected socket");
		return -1;
	}

	if(s->server) {
		LOG_ERROR("Cannot write to server socket");
		return -1;
	}

	if((ret = recv(s->fd, buffer, size - 1, 0)) == 0) { // connection reset by peer
		LOG_INFO("Connection on socket %d reset by peer", s->fd);
		disconnectSocket(s);
		return -1;
	} else if(ret < 0) {
#ifdef WIN32
		if(WSAGetLastError() == WSAEWOULDBLOCK) {
#else
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
			return 0; // The socket is non-blocking and we've got nothing back
		}

		LOG_SYSTEM_ERROR("Failed to read from socket %d", s->fd);
		return -1;
	}

	((char *) buffer)[ret] = '\0'; // recv is not null terminated

	return ret;
}

/**
 * Checks a server socket for new connections.
 *
 * @param server		the server socket
 * @param client		out parameter referencing the socket to the connected client
 * @return ServerSocketStatus
 */
API ServerSocketStatus serverSocketAccept(Socket *server, Socket **client)
{
	*client = NULL;

	struct sockaddr_storage remoteAddr;
	socklen_t remoteAddrSize = sizeof(remoteAddr);

	int ret = accept(server->fd, (struct sockaddr *)&remoteAddr, &remoteAddrSize);

	if(ret == -1) {
#ifdef WIN32
		if(WSAGetLastError() == WSAEWOULDBLOCK) {
			return SERVER_SOCKET_NO_CONNECTION;
		}
#else
		if(errno == EAGAIN) {
			return SERVER_SOCKET_NO_CONNECTION;
		}
#endif
		server->connected = false;

		return SERVER_SOCKET_ERROR;
	}

	Socket *clientSock = ALLOCATE_OBJECT(Socket);
	clientSock->fd = ret;
	clientSock->connected = true;
	clientSock->host = NULL;
	clientSock->port = NULL;
	clientSock->server = false;

	*client = clientSock;

	return SERVER_SOCKET_NEW_CONNECTION;
}

/**
 * Sets a socket to non-blocking I/O mode
 *
 * @param fd		the descriptor of the socket to set non-blocking
 * @result			true if successful
 */
API bool setSocketNonBlocking(int fd)
{
#ifdef WIN32
	unsigned long nbmode = 1;
	if(ioctlsocket(fd, FIONBIO, &nbmode) != 0) {
		LOG_WARNING("ioctrlsocket failed on fd %d", fd);
		return false;
	}
#else
	int flags;

	if((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		flags = 0; // if fcntl fails, set flags to zero
	}

	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
		LOG_WARNING("fcntl failed on fd %d", fd);
		return false;
	}
#endif

	return true;
}
