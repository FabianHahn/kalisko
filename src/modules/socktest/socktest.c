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

#define BUF 4096
#define REQUEST "GET / HTTP/1.1\nHost: www.kalisko.org\nConnection: close\n\n"
#define ANSWER "Hello there!\nThis is the Kalisko socktest module and client connections are apparently working.\nBye bye :-)\n"
static Socket *server;

MODULE_NAME("socktest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module shows the socket API in action");
MODULE_VERSION(0, 2, 4);
MODULE_BCVERSION(0, 2, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 4, 4));

HOOK_LISTENER(sample_read);
HOOK_LISTENER(sample_disconnect);
HOOK_LISTENER(sample_accept);

MODULE_INIT
{
	HOOK_ATTACH(socket_read, sample_read);
	HOOK_ATTACH(socket_disconnect, sample_disconnect);
	HOOK_ATTACH(socket_accept, sample_accept);

	Socket *http = $(Socket *, socket, createClientSocket)("www.kalisko.org", "http");
	$(bool, socket, connectSocket)(http);
	$(bool, socket, socketWriteRaw)(http, REQUEST, sizeof(REQUEST));
	$(bool, socket, enableSocketPolling)(http);

	server = $(Socket *, socket, createServerSocket)("1337");
	if(!$(bool, socket, connectSocket)(server)) {
		return false;
	}
	$(bool, socket, enableSocketPolling)(server);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(socket_read, sample_read);
	HOOK_DETACH(socket_disconnect, sample_disconnect);
	HOOK_DETACH(socket_accept, sample_accept);

	$(bool, socket, freeSocket)(server);
}

HOOK_LISTENER(sample_read)
{
	Socket *s = HOOK_ARG(Socket *);
	char *read = HOOK_ARG(char *);

	if(s != NULL) {
		printf("%s", read);
		fflush(stdout);
	}
}

HOOK_LISTENER(sample_disconnect)
{
	Socket *s = HOOK_ARG(Socket *);

	$(void, socket, freeSocket)(s);
}

HOOK_LISTENER(sample_accept)
{
	Socket *srv = HOOK_ARG(Socket *);
	Socket *s = HOOK_ARG(Socket *);

	if(srv == NULL) {
		return;
	}

	$(bool, socket, socketWriteRaw)(s, ANSWER, sizeof(ANSWER));
	$(bool, socket, disconnectSocket)(s);
}
