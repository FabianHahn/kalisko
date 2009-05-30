/**
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Kalisko Developers nor the names of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef WIN32
#include <winsock.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "socket.h"
#include "hooks.h"

HOOK_LISTENER(log);

#define BUF 4096
#define REQUEST "GET / HTTP/1.1\nHost: www.kalisko.org\nConnection: close\n\n"

int main(int argc, char **argv)
{
	initHooks();
	initLog();

#ifdef WIN32
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        logError("WSAStartup failed");
        exit(EXIT_FAILURE);
    }
#endif

	HOOK_ATTACH(log, log);

	Socket *sock = createClientSocket("www.kalisko.org", "http");
	connectSocket(sock);

	socketWriteRaw(sock, REQUEST, sizeof(REQUEST));

	while(sock->connected) {
		char buffer[BUF];

		memset(buffer, 0, BUF);

		socketReadRaw(sock, buffer, BUF);

		printf("%s", buffer);
	}

	freeSocket(sock);

#ifdef WIN32
	WSACleanup();
#endif

	freeHooks();

	return EXIT_SUCCESS;
}

HOOK_LISTENER(log) // this will be removed as soon as we have a real log module
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	switch(type) {
		case LOG_ERROR:
			fprintf(stderr, "(error) %s\n", message);
		break;
		case LOG_WARNING:
			fprintf(stderr, "(warning) %s\n", message);
		break;
		case LOG_INFO:
			fprintf(stderr, "(info) %s\n", message);
		break;
		case LOG_DEBUG:
			fprintf(stderr, "(debug) %s\n", message);
		break;
	}
}
