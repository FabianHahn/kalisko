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

MODULE_INIT
{
	testSocket();

	return true;
}

MODULE_FINALIZE
{

}

static void testSocket()
{
	Socket *sock = $(Socket *, socket, createClientSocket)("www.kalisko.org", "http");
	$(bool, socket, connectSocket)(sock);

	$(bool, socket, socketWriteRaw)(sock, REQUEST, sizeof(REQUEST));

	while(sock->connected) {
		char buffer[BUF];

		memset(buffer, 0, BUF);

		$(int, socket, socketReadRaw)(sock, buffer, BUF);

		printf("%s", buffer);
	}

	$(bool, socket, freeSocket)(sock);
}
