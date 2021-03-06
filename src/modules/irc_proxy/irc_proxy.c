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


#include <glib.h>
#include <stdio.h>
#include <stdarg.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/irc/irc.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/string_util/string_util.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/config/config.h"
#include "modules/event/event.h"
#define API
#include "irc_proxy.h"

MODULE_NAME("irc_proxy");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC proxy module relays IRC traffic from and to an IRC server through a server socket");
MODULE_VERSION(0, 3, 11);
MODULE_BCVERSION(0, 3, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 5, 0), MODULE_DEPENDENCY("socket", 0, 4, 4), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("event", 0, 1, 2));

static void freeIrcProxyClient(void *client_p, void *quitmsg_p);
static void checkForBufferLine(IrcProxyClient *client);

/**
 * A hash table associating proxy names with their corresponding IrcProxy objects
 */
static GHashTable *proxies;

/**
 * A hash table that associates remote IrcConnection objects with their corresponding IrcProxy objects
 */
static GHashTable *proxyConnections;

/**
 * Hash table that associates client Socket objects with their corresponding IrcProxyClient object
 */
static GHashTable *clients;

/**
 * IRC proxy server socket on which the module listens for new IRC proxy client connections
 */
static Socket *server;

static void listener_remoteLine(void *subject, const char *event, void *data, va_list args);
static void listener_clientAccept(void *subject, const char *event, void *data, va_list args);
static void listener_clientRead(void *subject, const char *event, void *data, va_list args);
static void listener_clientDisconnect(void *subject, const char *event, void *data, va_list args);
static void listener_clientLine(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	proxies = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	proxyConnections = g_hash_table_new(NULL, NULL);
	clients = g_hash_table_new(NULL, NULL);

	Store *config;
	char *port = "6677";

	if((config = getConfigPath("irc/proxy/port")) != NULL && config->type == STORE_STRING) {
		port = config->content.string;
	} else {
		logNotice("Could not determine config value irc/proxy/port, using default of '%s'", port);
	}

	// Create and connect our listening server socket
	server = createServerSocket(port);

	attachEventListener(server, "accept", NULL, &listener_clientAccept);

	if(!connectSocket(server)) {
		logError("Failed to connect IRC proxy server socket on port %s, aborting", port);
		return false;
	}

	if(!enableSocketPolling(server)) {
		logError("Failed to enable polling for IRC proxy server socket on port %s, aborting", port);
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	GList *proxylist = g_hash_table_get_values(proxies);
	for(GList *iter = proxylist; iter != NULL; iter = iter->next) {
		freeIrcProxy(iter->data);
	}
	g_list_free(proxylist);

	g_hash_table_destroy(proxies);
	g_hash_table_destroy(proxyConnections);
	g_hash_table_destroy(clients);

	detachEventListener(server, "accept", NULL, &listener_clientAccept);

	// Free the server socket
	freeSocket(server);
}

static void listener_remoteLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	IrcProxy *proxy;
	if((proxy = g_hash_table_lookup(proxyConnections, irc)) != NULL) { // One of our proxy servers got a new remote line
		for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) { // iterate over all clients
			IrcProxyClient *client = iter->data; // retrieve client

			if(client->authenticated && g_strcmp0(message->command, "PING") != 0) { // only relay to authenticated clients, don't relay ping messages
				proxyClientIrcSend(client, "%s", message->raw_message); // relay message to client
			}
		}
	}
}

static void listener_clientAccept(void *subject, const char *event, void *data, va_list args)
{
	Socket *listener = subject;
	Socket *client = va_arg(args, Socket *);

	if(listener == server) { // new IRC proxy client
		enableSocketPolling(client); // also poll the new socket

		logNotice("New relay client %d on IRC proxy server", client->fd);

		IrcProxyClient *pc = ALLOCATE_OBJECT(IrcProxyClient);
		pc->proxy = NULL;
		pc->socket = client;
		pc->authenticated = false;
		pc->ibuffer = g_string_new("");

		attachEventListener(pc, "line", NULL, &listener_clientLine);
		attachEventListener(client, "read", NULL, &listener_clientRead);
		attachEventListener(client, "disconnect", NULL, &listener_clientDisconnect);

		g_hash_table_insert(clients, client, pc); // connect the client socket to the proxy client object

		proxyClientIrcSend(pc, ":kalisko.proxy NOTICE AUTH :*** Welcome to the Kalisko IRC proxy server! Please use the %cPASS [id]:[password]%c command to authenticate...", (char) 2, (char) 2);
	}
}

static void listener_clientRead(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;
	char *message = va_arg(args, char *);

	IrcProxyClient *client;
	if((client = g_hash_table_lookup(clients, socket)) != NULL) {
		g_string_append(client->ibuffer, message);
		checkForBufferLine(client);
	}
}

static void listener_clientDisconnect(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;

	IrcProxyClient *client;
	if((client = g_hash_table_lookup(clients, socket)) != NULL) { // one of our proxy clients disconnected
		logNotice("IRC proxy client %d disconnected", client->socket->fd);

		if(client->proxy != NULL) { // check if the client is already associated to a proxy
			g_queue_remove(client->proxy->clients, client); // remove the client from its irc proxy
		}

		freeIrcProxyClient(client, "Bye");
	}
}

static void listener_clientLine(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if(!client->authenticated) { // not yet authenticated, wait for password
		if(g_strcmp0(message->command, "PASS") == 0) {
			char *password = NULL;

			if(message->params_count > 0 && message->params != NULL && message->params[0] != 0) { // password seems to be in the first param
				password = message->params[0];
			} else { // password must be in trailing space
				password = message->trailing;
			}

			if(password != NULL) {
				char **parts = g_strsplit(password, ":", 0);
				int count = 0;
				while(parts[count] != NULL) { // count parts
					count++;
				}

				if(count >= 2) { // there are at least two parts
					char *name = parts[0];
					IrcProxy *proxy;

					if((proxy = g_hash_table_lookup(proxies, name)) != NULL) { // it's a valid ID
						if(g_strcmp0(proxy->password, parts[1]) == 0) { // the password also matches
							logNotice("IRC proxy client %d authenticated successfully to IRC proxy '%s'", client->socket->fd, name);
							client->authenticated = true;

							// associate client and proxy
							client->proxy = proxy;
							g_queue_push_head(proxy->clients, client);

							proxyClientIrcSend(client, ":%s 001 %s :You were successfully authenticated and are now connected to the IRC server", proxy->irc->socket->host, proxy->irc->nick);
							proxyClientIrcSend(client, ":%s 251 %s :There are %d clients online on this bouncer", client->proxy->irc->socket->host, proxy->irc->nick, g_queue_get_length(proxy->clients));
							triggerEvent(proxy, "client_authenticated", client);
						} else {
							proxyClientIrcSend(client, ":kalisko.proxy NOTICE AUTH :*** Login incorrect for IRC proxy ID %c%s%c", (char) 2, name, (char) 2);
						}
					} else {
						proxyClientIrcSend(client, ":kalisko.proxy NOTICE AUTH :*** Invalid IRC proxy ID %c%s%c", (char) 2, name, (char) 2);
					}
				}

				g_strfreev(parts);
			}
		}
	} else if(g_strcmp0(message->command, "PING") == 0) { // reply to pings
		if(message->trailing != NULL) {
			proxyClientIrcSend(client, "PONG :%s", message->trailing);
		}
	} else if(g_strcmp0(message->command, "USER") == 0) { // prevent user command from being passed through
		return;
	} else if(g_strcmp0(message->command, "QUIT") == 0) { // client disconnects
		logInfo("IRC proxy client %d sent QUIT message, disconnecting...", client->socket->fd);
		disconnectSocket(client->socket);
	} else {
		if((g_strcmp0(message->command, "PRIVMSG") == 0 || g_strcmp0(message->command, "NOTICE") == 0) && message->params_count > 0) { // potential filtered command
			for(GList *iter = client->proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
				if(g_strcmp0(iter->data, message->params[0]) == 0) { // target matches, so don't relay!
					return;
				}
			}
		}

		// Relay message to IRC server
		ircSend(client->proxy->irc, "%s", message->raw_message);
	}
}

API IrcProxy *createIrcProxy(char *name, IrcConnection *irc, char *password)
{
	if(g_hash_table_lookup(proxies, name) != NULL) { // IRC proxy with that ID already exists
		logError("Trying to create IRC proxy with already taken name '%s', aborting", name);
		return NULL;
	}

	if(g_hash_table_lookup(proxyConnections, irc) != NULL) { // there is already a proxy for this connection
		logError("Trying to create IRC proxy for already proxied IRC connection with socket %d, aborting", irc->socket->fd);
		return NULL;
	}

	IrcProxy *proxy = ALLOCATE_OBJECT(IrcProxy);
	proxy->name = strdup(name);
	proxy->irc = irc;
	proxy->password = strdup(password);
	proxy->clients = g_queue_new();
	proxy->relay_exceptions = g_queue_new();

	attachEventListener(irc, "line", NULL, &listener_remoteLine);

	g_hash_table_insert(proxies, strdup(name), proxy); // clone name again to make it independent of the IrcProxy object
	g_hash_table_insert(proxyConnections, irc, proxy);

	return proxy;
}

API GList *getIrcProxies()
{
	return g_hash_table_get_values(proxies);
}

API IrcProxy *getIrcProxyByIrcConnection(IrcConnection *irc)
{
	return g_hash_table_lookup(proxyConnections, irc);
}

API IrcProxy *getIrcProxyByName(char *name)
{
	return g_hash_table_lookup(proxies, name);
}

API IrcProxyClient *getIrcProxyClientBySocket(Socket *socket)
{
	return g_hash_table_lookup(clients, socket);
}

API void freeIrcProxy(IrcProxy *proxy)
{
	detachEventListener(proxy->irc, "line", NULL, &listener_remoteLine);

	g_hash_table_remove(proxies, proxy->name);
	g_hash_table_remove(proxyConnections, proxy->irc);

	// Free relay exceptions
	for(GList *iter = proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
		free(iter->data); // free the exception target
	}

	g_queue_free(proxy->relay_exceptions); // free the exceptions list

	g_queue_foreach(proxy->clients, &freeIrcProxyClient, "IRC proxy server going down");
	g_queue_free(proxy->clients);
	free(proxy->name);
	free(proxy->password);
	free(proxy);
}

API void addIrcProxyRelayException(IrcProxy *proxy, char *exception)
{
	g_queue_push_tail(proxy->relay_exceptions, strdup(exception));
}

API bool delIrcProxyRelayException(IrcProxy *proxy, char *exception)
{
	for(GList *iter = proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
		if(g_strcmp0(iter->data, exception) == 0) { // this is it
			free(iter->data); // free the string
			g_queue_remove(proxy->relay_exceptions, iter->data); // remove it from the exceptions list
			return true;
		}
	}

	return false;
}

API bool hasIrcProxyRelayException(IrcProxy *proxy, char *exception)
{
	for(GList *iter = proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
		if(g_strcmp0(iter->data, exception) == 0) { // this is it
			return true;
		}
	}

	return false;
}

API bool proxyClientIrcSend(IrcProxyClient *client, char *message, ...)
{

	va_list va;
	char buffer[IRC_SEND_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, IRC_SEND_MAXLEN, message, va);

	if(!client->socket->connected) {
		logError("Trying to send to disconnected IRC proxy client, aborting: %s", buffer);
		return false;
	}

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = socketWriteRaw(client->socket, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

/**
 * A GFunc to free an IRC proxy client. Note that this doesn't remove the IRC proxy client from its parent proxy's client list
 *
 * @param client_p		a pointer to the IRC proxy client to free
 * @param quitmsg_p		a quit message to send to the disconnecting client
 */
static void freeIrcProxyClient(void *client_p, void *quitmsg_p)
{
	IrcProxyClient *client = client_p;
	char *quitmsg = quitmsg_p;

	if(client->socket->connected && quitmsg != NULL) { // Send quit message
		proxyClientIrcSend(client, "QUIT :%s", quitmsg);
	}

	triggerEvent(client->proxy, "client_disconnected", client);
	detachEventListener(client, "line", NULL, &listener_clientLine);
	detachEventListener(client->socket, "read", NULL, &listener_clientRead);
	detachEventListener(client->socket, "disconnect", NULL, &listener_clientDisconnect);

	g_hash_table_remove(clients, client->socket); // remove ourselves from the irc proxy client sockets table

	freeSocket(client->socket); // free the socket
	g_string_free(client->ibuffer, true); // free the input buffer

	free(client); // actually free the client
}

/**
 * Checks for a new newline terminated line in the buffer and parses it
 *
 * @param client			the IRC proxy client to check for buffer lines
 */
static void checkForBufferLine(IrcProxyClient *client)
{
	char *message = client->ibuffer->str;

	if(strstr(message, "\n") != NULL) { // message has at least one newline
		g_string_free(client->ibuffer, false);
		stripDuplicateNewlines(message); // remove duplicate newlines, since server could send \r\n
		char **parts = g_strsplit(message, "\n", 0);
		int count = 0;
		for(char **iter = parts; *iter != NULL; iter++) {
			count++;
		}

		for(int i = 0; i < count - 1; i++) { // Don't trigger the last part, it's not yet complete
			char *part = parts[i];
			if(strlen(part) > 0) {
				IrcMessage *ircMessage = parseIrcMessage(part);

				if(ircMessage != NULL) {
					triggerEvent(client, "line", ircMessage);
					freeIrcMessage(ircMessage);
				}
			}
		}

		client->ibuffer = g_string_new(parts[count-1]); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
