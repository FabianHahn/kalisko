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

#include <memory.h>
#include <stdio.h>
#include <glib.h>

#include "dll.h"
#include "hooks.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"

#include "api.h"

static void testSocket();

#define BUF 4096
#define REQUEST "GET / HTTP/1.1\nHost: www.kalisko.org\nConnection: close\n\n"

MODULE_NAME("socktest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module shows the socket API in action");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 1, 2));

HOOK_LISTENER(sample_read);
HOOK_LISTENER(sample_disconnect);
HOOK_LISTENER(sample_client_connected);

MODULE_INIT
{
	testSocket();
	HOOK_ATTACH(socket_read, sample_read);
	HOOK_ATTACH(socket_disconnect, sample_disconnect);
	HOOK_ATTACH(socket_client_connected, sample_client_connected);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(socket_read, sample_read);
	HOOK_DETACH(socket_disconnect, sample_disconnect);
}

HOOK_LISTENER(sample_read)
{
	Socket *s = HOOK_ARG(Socket *);
	char *read = HOOK_ARG(char *);

	printf("\n--%d--\n%s\n--------\n", s->fd, read);
}

HOOK_LISTENER(sample_disconnect)
{
	Socket *s = HOOK_ARG(Socket *);

	$(bool, socket, freeSocket)(s);
}

HOOK_LISTENER(sample_client_connected)
{
	Socket *s = HOOK_ARG(Socket *);

	$(bool, socket, setSocketNonBlocking)(s->fd);
	$(bool, socket, enableSocketPolling)(s);
}

static void testSocket()
{
	Socket *sock = $(Socket *, socket, createClientSocket)("www.smf68.ch", "http");
	$(bool, socket, connectSocket)(sock);
	$(bool, socket, enableSocketPolling)(sock);

	$(bool, socket, socketWriteRaw)(sock, REQUEST, sizeof(REQUEST));

	Socket *server = $(Socket *, socket, createServerSocket)("23");
	$(bool, socket, connectSocket)(server);
	$(bool, socket, enableSocketPolling)(server);
}
