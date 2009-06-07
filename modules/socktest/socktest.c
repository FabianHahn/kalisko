#include <memory.h>
#include <stdio.h>
#include <glib.h>

#include "dll.h"
#include "modules/socket/socket.h"

#include "api.h"

static void testSocket();

#define BUF 4096
#define REQUEST "GET / HTTP/1.1\nHost: www.kalisko.org\nConnection: close\n\n"

API bool module_init()
{
	testSocket();

	return true;
}

API void module_finalize()
{

}

API GList *module_depends()
{
	return g_list_append(NULL, "socket");
}

static void testSocket()
{
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
}
