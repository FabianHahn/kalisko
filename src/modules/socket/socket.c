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
#include <assert.h> // assert
#include <string.h> // strerror, strdup

#include "dll.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"
#include "module.h"
#include "modules/config/config.h"
#include "modules/store/path.h"
#include "modules/event/event.h"

#define API
#include "socket.h"
#include "poll.h"
#include "util.h"

#define IP_STR_LEN 16

MODULE_NAME("socket");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The socket module provides an API to establish network connections and transfer data over them");
MODULE_VERSION(0, 7, 3);
MODULE_BCVERSION(0, 4, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("event", 0, 1, 2));

static int connectionTimeout = 10; // set default connection timeout to 10 seconds

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

	Store *configPollInterval = $(Store *, store, getStorePath)($(Store *, config, getConfig)(), "socket/pollInterval");
	if(configPollInterval != NULL && configPollInterval->type == STORE_INTEGER) {
		pollInterval = configPollInterval->content.integer;
	} else {
		LOG_INFO("Could not determine config value socket/pollInterval, using default");
	}

	Store *configConnectionTimeout = $(Store *, store, getStorePath)($(Store *, config, getConfig)(), "socket/connectionTimeout");
	if(configConnectionTimeout != NULL && configConnectionTimeout->type == STORE_INTEGER) {
		connectionTimeout = configConnectionTimeout->content.integer;
	} else {
		LOG_INFO("Could not determine config value socket/connectionTimeout, using default");
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
 * @param port		the port to connect to
 * @result			the created socket
 */
API Socket *createClientSocket(char *host, char *port)
{
	Socket *s = ALLOCATE_OBJECT(Socket);

	s->fd = -1;
	s->host = strdup(host);
	s->port = strdup(port);
	s->type = SOCKET_CLIENT;
	s->connected = false;
	s->custom = NULL;
#ifdef WIN32
	s->in = NULL;
	s->out = NULL;
#endif

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
	s->type = SOCKET_SERVER;
	s->connected = false;
	s->custom = NULL;
#ifdef WIN32
	s->in = NULL;
	s->out = NULL;
#endif

	return s;
}

/**
 * Create a shell socket
 *
 * @param args			the shell command arguments, terminated by NULL
 * @result				the created socket
 */
API Socket *createShellSocket(char **args)
{
	Socket *s = ALLOCATE_OBJECT(Socket);

	s->fd = -1;
	s->host = NULL;
	s->port = NULL;
	s->custom = g_strdupv(args);
	s->type = SOCKET_SHELL;
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

	struct addrinfo hints;
	struct addrinfo *server;
	int ret;

#ifdef WIN32
	char *command;
#else
	int fds[2];
#endif

	switch(s->type) {
		case SOCKET_SERVER:
		case SOCKET_SERVER_BLOCK:
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET; // use IPv4
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_PASSIVE; // fill in the IP automaticly

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
			if(setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (void *) &param, sizeof(int)) == -1) {
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

			if(s->type != SOCKET_SERVER_BLOCK) {
				if(!setSocketNonBlocking(s->fd)) {
					LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
					freeSocket(s);
					return false;
				}
			}

			freeaddrinfo(server);

			LOG_DEBUG("Connected server socket %d on port %s", s->fd, s->port);
		break;
		case SOCKET_CLIENT:
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

			s->connected = true;

			if(!setSocketNonBlocking(s->fd)) {
				LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
				return false;
			}

			if(connect(s->fd, server->ai_addr, server->ai_addrlen) < 0) { // try to connect socket
#ifdef WIN32
				if(WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK) {
#else
				if(errno == EINPROGRESS) {
#endif
					LOG_DEBUG("Socket %d delayed connection, waiting...", s->fd);
					do {
						// Initialize timeout
						struct timeval tv;
						tv.tv_sec = connectionTimeout;
						tv.tv_usec = 0;

						// Initialize write fd set
						fd_set fdset;
						FD_ZERO(&fdset);
						FD_SET(s->fd, &fdset);

						// Select socket for write flag (connected)
						int ret;
#ifdef WIN32
						if((ret = select(s->fd + 1, NULL, &fdset, NULL, &tv)) < 0 && WSAGetLastError() != WSAEINTR) {
#else
						if((ret = select(s->fd + 1, NULL, &fdset, NULL, &tv)) < 0 && errno != EINTR) {
#endif
							LOG_SYSTEM_ERROR("Error selecting socket %d for write flag (connected)", s->fd);
							freeaddrinfo(server);
							disconnectSocket(s);
							return false;
						} else if(ret > 0) {
							// Socket selected for write, check if we're indeed connected
							int valopt;
							socklen_t lon = sizeof(int);
							if(getsockopt(s->fd, SOL_SOCKET, SO_ERROR, (void*) (&valopt), &lon) < 0) {
								LOG_SYSTEM_ERROR("getsockopt() failed on socket %d", s->fd);
								freeaddrinfo(server);
								disconnectSocket(s);
								return false;
							} else if(valopt != 0) { // There was a connection error
								LOG_SYSTEM_ERROR("Delayed connection on socket %d failed", s->fd);
								freeaddrinfo(server);
								disconnectSocket(s);
								return false;
							}

							break; // Socket connected, break out of loop
						} else {
							LOG_ERROR("Connection for socket %d exceeded timeout of %d seconds, disconnecting", s->fd, connectionTimeout);
							freeaddrinfo(server);
							disconnectSocket(s);
							return false;
						}
					} while(true);
				} else {
#ifdef WIN32
					char *error = g_win32_error_message(WSAGetLastError());
					LOG_ERROR("Connection for socket %d failed: %s", s->fd, error);
					free(error);
#else
					LOG_SYSTEM_ERROR("Connection for socket %d failed", s->fd);
#endif
					freeaddrinfo(server);
					disconnectSocket(s);
					return false;
				}
			}

			freeaddrinfo(server);

			LOG_DEBUG("Connected client socket %d to %s:%s", s->fd, s->host, s->port);
		break;
		case SOCKET_SERVER_CLIENT:
			LOG_ERROR("Cannot connect to server client socket, aborting");
			return false;
		break;
		case SOCKET_SHELL:
#ifdef WIN32
			command = NULL;

			// Force pipe handles to be inherited
			SECURITY_ATTRIBUTES attr;
			attr.nLength = sizeof(SECURITY_ATTRIBUTES);
			attr.bInheritHandle = TRUE;
			attr.lpSecurityDescriptor = NULL;

			HANDLE handles[4];

			// stdout pipe
			if(!CreatePipe(&handles[0], &handles[1], &attr, 0)) {
				char *error = g_win32_error_message(GetLastError());
				LOG_ERROR("CreatePipe() failed: %s", error);
				free(error);
				return false;
			}

			if(!SetHandleInformation(handles[0], HANDLE_FLAG_INHERIT, 0)) {
				char *error = g_win32_error_message(GetLastError());
				LOG_ERROR("SetHandleInformation() failed: %s", error);
				free(error);
				return false;
			}

			// stdin pipe
			if(!CreatePipe(&handles[2], &handles[3], &attr, 0)) {
				char *error = g_win32_error_message(GetLastError());
				LOG_ERROR("SetHandleInformation() failed: %s", error);
				free(error);
				return false;
			}

			if(!SetHandleInformation(handles[3], HANDLE_FLAG_INHERIT, 0)) {
				char *error = g_win32_error_message(GetLastError());
				LOG_ERROR("SetHandleInformation() failed: %s", error);
				free(error);
				return false;
			}

			// Create the child process.
			PROCESS_INFORMATION procInfo;
			STARTUPINFO startInfo;
			ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
			ZeroMemory(&startInfo, sizeof(STARTUPINFO));
			startInfo.cb = sizeof(STARTUPINFO);
			startInfo.hStdError = handles[1];
			startInfo.hStdOutput = handles[1];
			startInfo.hStdInput = handles[2];
			startInfo.dwFlags |= STARTF_USESTDHANDLES;

			command = g_strjoinv(" ", s->custom);

			if(!CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &startInfo, &procInfo)) {
				char *error = g_win32_error_message(GetLastError());
				LOG_ERROR("CreateProcess() failed: %s", error);
				free(error);
				free(command);
				return false;
			} else {
				CloseHandle(procInfo.hProcess);
				CloseHandle(procInfo.hThread);
			}

			free(command);

			CloseHandle(handles[1]);
			CloseHandle(handles[2]);

			int writefd;

			if((writefd = _open_osfhandle((intptr_t) handles[3], _O_APPEND)) == -1) {
				LOG_SYSTEM_ERROR("_open_osfhandle() failed");
				return false;
			}

			int readfd;

			if((readfd = _open_osfhandle((intptr_t) handles[0], _O_RDONLY | _O_BINARY)) == -1) {
				LOG_SYSTEM_ERROR("_open_osfhandle() failed");
				return false;
			}

			if((s->out = _fdopen(writefd, "a")) == NULL) {
				LOG_SYSTEM_ERROR("_fdopen() failed");
				return false;
			}

			if((s->in = _fdopen(readfd, "rb")) == NULL) {
				LOG_SYSTEM_ERROR("_fdopen() failed");
				return false;
			}

			s->fd = writefd;
			s->connected = true;
#else
			// Create socket pair
			if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
				LOG_SYSTEM_ERROR("socketpair() failed for shell socket");
				return false;
			}

			// Assign one side of the socket pair to our socket
			s->fd = fds[0];

			if(!setSocketNonBlocking(s->fd)) {
				LOG_SYSTEM_ERROR("Failed to set socket non-blocking");
				freeSocket(s);
				return false;
			}

			pid_t pid;

			// Fork and assign the other side of the socket pair to the executed application
			if((pid = fork()) < 0) { // fork failed
				LOG_SYSTEM_ERROR("fork() failed for shell socket, aborting");
				freeSocket(s);
				return false;
			} else if(pid == 0) { // Child
				if(dup2(fds[1], 0) == 0 && dup2(fds[1], 1) == 1 && dup2(fds[1], 2) == 2)
				{
					// FIXME: necessary to close all other sockets here?
					execvp(((char * const *) s->custom)[0], (char * const *) s->custom);
				}

				exit(EXIT_FAILURE);
			}

			// Close the other side of the socket pair for the parent
			close(fds[1]);

			s->connected = true;
#endif

			LOG_DEBUG("Connected shell socket %d", s->fd);
		break;
	}

	return true;
}

/**
 * Disconnects a socket. Call this function to get rid of a socket inside a socket_read hook, then free it inside a socket_disconnect listener.
 * See the documentation of freeSocket for further details on this issue.
 * @see freeSocket
 *
 * @param s			the socket to disconnect
 * @result			true if successful, false on error
 */
API bool disconnectSocket(Socket *s)
{
	LOG_DEBUG("Disconnecting socket %d", s->fd);

	if(s->connected) {
#ifdef WIN32
		if(s->in != NULL) {
			fclose(s->in);
			fclose(s->out);
		} else if(closesocket(s->fd) != 0) {
#else
		if(close(s->fd) != 0) {
#endif
			LOG_SYSTEM_ERROR("Failed to close socket %d", s->fd);
			return false;
		}

		s->connected = false;

		return true;
	} else {
		LOG_ERROR("Cannot disconnect already disconnected socket %d", s->fd);
		return false;
	}
}

/**
 * Frees a socket. Note that this function MUST NOT be called from a (descendent of a) socket_read hook since further listeners expect the socket
 * to still be existing. If you want to get rid of a socket after a read event, listen to the socket_disconnect hook and disconnect it with disconnectSocket().
 * Then, free it inside the socket_disconnect hook using this function. If you don't want to adhere to this rule, you might as well shoot yourself in the foot.
 * @see disconnectSocket
 *
 * @param s			the socket to free
 */
API void freeSocket(Socket *s)
{
	// disconnect the socket first if it's still connected
	if(s->connected) {
		disconnectSocket(s);
	}

	// Disable socket polling for this socket. In usual cases, this is not necessary and is done automatically by the polling engine, but do this just to be sure no orphaned sockets remain in the polling table
	disableSocketPolling(s);

	if(s->host) {
		free(s->host);
	}

	if(s->port) {
		free(s->port);
	}

	if(s->type == SOCKET_CLIENT) {
		free(s->custom);
	}

	if(s->type == SOCKET_SHELL) {
		g_strfreev(s->custom);
	}

	free(s);
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

	if(s->type == SOCKET_SERVER) {
		LOG_ERROR("Cannot write to server socket %d", s->fd);
		return false;
	}

	while(left > 0) {
#ifdef WIN32
		if(s->in != NULL) {
			if((ret = fwrite(buffer, sizeof(char), left, s->in)) > 0) {
				left -= ret;
				buffer += ret;
			} else {
				LOG_SYSTEM_ERROR("Failed to write to pipe socket %d", s->fd);
				return false;
			}
		} else
#endif
#ifdef MSG_NOSIGNAL // prevent SIGPIPE if we can...
		if((ret = send(s->fd, buffer, left, MSG_NOSIGNAL)) > 0) { // wrote ret characters
#else
		if((ret = send(s->fd, buffer, left, 0)) > 0) { // wrote ret characters
#endif
			left -= ret;
			buffer += ret;
		} else if(errno == EINTR) { // interrupted
			continue;
		} else if(errno == EPIPE) { // broken pipe means the connection broke down but we didn't get the disconnect event yet
			LOG_INFO("Broken pipe for socket %d on write, disconnecting...", s->fd);
			disconnectSocket(s);
			return false;
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

	if(s->type == SOCKET_SERVER || s->type == SOCKET_SERVER_BLOCK) {
		LOG_ERROR("Cannot write to server socket");
		return -1;
	}

#ifdef WIN32
	if(s->in != NULL) {
		if((ret = fread(buffer, sizeof(char), size - 1, s->in)) == 0) {
			if(feof(s->in)) {
				LOG_INFO("EOF on pipe socket %d", s->fd);
			} else {
				LOG_SYSTEM_ERROR("Failed to read from pipe socket %d", s->fd);
			}

			disconnectSocket(s);
			return -1;
		}
	} else
#endif

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

#ifdef WIN32
		char *error = g_win32_error_message(GetLastError());
		LOG_ERROR("Failed to read from socket %d: %s", s->fd, error);
		free(error);
#else
		LOG_SYSTEM_ERROR("Failed to read from socket %d", s->fd);
#endif
		disconnectSocket(s);
		return -1;
	}

	((char *) buffer)[ret] = '\0'; // recv is not null terminated

	return ret;
}

/**
 * Accepts a client socket from a listening server socket
 *
 * @param server		the server socket
 * @return Socket		the accepted socket
 */
API Socket *socketAccept(Socket *server)
{
	struct sockaddr_in address;
	socklen_t addressSize = sizeof(struct sockaddr_in);

	int fd;

	if((fd = accept(server->fd, (struct sockaddr *) &address, &addressSize)) == -1) {
#ifdef WIN32
		if(WSAGetLastError() == WSAEWOULDBLOCK) {
#else
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
		} else {
			LOG_SYSTEM_ERROR("Failed to accept client socket from server socket %d", server->fd);
			disconnectSocket(server);
		}

		return NULL;
	}

	if(!setSocketNonBlocking(fd)) {
		return NULL;
	}

	GString *ip = ip2str(address.sin_addr.s_addr);
	GString *port = g_string_new("");
	g_string_append_printf(port, "%d", ntohs(address.sin_port));

	Socket *client = ALLOCATE_OBJECT(Socket);
	client->fd = fd;
	client->connected = true;
	client->host = ip->str;
	client->port = port->str;
	client->type = SOCKET_SERVER_CLIENT;
#ifdef WIN32
	client->in = NULL;
	client->out = NULL;
#endif

	LOG_DEBUG("Incoming connection %d from %s:%s on server socket %d", fd, client->host, client->port, server->fd);

	g_string_free(ip, false);
	g_string_free(port, false);

	return client;
}
