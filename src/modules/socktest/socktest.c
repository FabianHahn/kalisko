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
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/event/event.h"

#include "api.h"

#define BUF 4096
#define REQUEST "GET / HTTP/1.1\nHost: www.kalisko.org\nConnection: close\n\n"
#define ANSWER "Hello there!\nThis is the Kalisko socktest module and client connections are apparently working.\nBye bye :-)\n"
static Socket *server;

MODULE_NAME("socktest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module shows the socket API in action");
MODULE_VERSION(0, 2, 5);
MODULE_BCVERSION(0, 2, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 4, 4), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_socketRead(void *subject, const char *event, void *data, va_list args);
static void listener_socketDisconnect(void *subject, const char *event, void *data, va_list args);
static void listener_socketAccept(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	Socket *http = $(Socket *, socket, createClientSocket)("www.kalisko.org", "http");
	$(bool, socket, connectSocket)(http);
	$(bool, socket, socketWriteRaw)(http, REQUEST, sizeof(REQUEST));
	$(bool, socket, enableSocketPolling)(http);
	$(void, event, attachEventListener)(http, "read", NULL, &listener_socketRead);
	$(void, event, attachEventListener)(http, "disconnect", NULL, &listener_socketDisconnect);

	server = $(Socket *, socket, createServerSocket)("1337");
	if(!$(bool, socket, connectSocket)(server)) {
		return false;
	}
	$(bool, socket, enableSocketPolling)(server);
	$(void, event, attachEventListener)(server, "accept", NULL, &listener_socketAccept);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(server, "accept", NULL, &listener_socketAccept);
	$(bool, socket, freeSocket)(server);
}

static void listener_socketRead(void *subject, const char *event, void *data, va_list args)
{
	Socket *s = subject;
	char *read = va_arg(args, char *);

	if(s != NULL) {
		printf("%s", read);
		fflush(stdout);
	}
}

static void listener_socketDisconnect(void *subject, const char *event, void *data, va_list args)
{
	Socket *s = subject;

	$(void, event, detachEventListener)(s, "read", NULL, &listener_socketRead);
	$(void, event, detachEventListener)(s, "disconnect", NULL, &listener_socketDisconnect);

	$(void, socket, freeSocket)(s);
}

static void listener_socketAccept(void *subject, const char *event, void *data, va_list args)
{
	Socket *srv = subject;
	Socket *s = va_arg(args, Socket *);

	if(srv == NULL) {
		return;
	}

	$(void, event, attachEventListener)(s, "disconnect", NULL, &listener_socketDisconnect);
	$(bool, socket, socketWriteRaw)(s, ANSWER, sizeof(ANSWER));
	$(bool, socket, disconnectSocket)(s);
}
