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
#define API
#include "socket.h"
#include "util.h"


API bool setSocketNonBlocking(int fd)
{
#ifdef WIN32
	unsigned long nbmode = 1;
	if(ioctlsocket(fd, FIONBIO, &nbmode) != 0) {
		logWarning("ioctrlsocket failed on fd %d", fd);
		return false;
	}
#else
	int flags;

	if((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		flags = 0; // if fcntl fails, set flags to zero
	}

	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
		logWarning("fcntl failed on fd %d", fd);
		return false;
	}
#endif

	return true;
}

API GString *ip2str(unsigned int ip)
{
	GString *ipstr = g_string_new("");
	g_string_append_printf(ipstr, "%i.%i.%i.%i", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
	return ipstr;
}

API bool closeSocket(Socket *s)
{
#ifdef WIN32
	if(s->in != NULL) {
		fclose(s->in);
		fclose(s->out);
	}
#endif

	if(s->fd >= 0) {
#ifdef WIN32
		if(closesocket(s->fd) != 0) {
#else
		if(close(s->fd) != 0) {
#endif
			logSystemError("Failed to close socket %d", s->fd);
			return false;
		}
	}

	return true;
}
