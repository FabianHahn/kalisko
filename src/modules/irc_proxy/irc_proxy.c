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
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/irc/irc.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/string_util/string_util.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"
#include "irc_proxy.h"

MODULE_NAME("irc_proxy");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC proxy module relays IRC traffic from and to an IRC server through a server socket");
MODULE_VERSION(0, 1, 13);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 2, 7), MODULE_DEPENDENCY("socket", 0, 4, 4), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0));

static void freeIrcProxyClient(void *client_p, void *quitmsg_p);
static void checkForBufferLine(IrcProxyClient *client);

static GHashTable *proxies;
static GHashTable *proxyConnections;
static GHashTable *clients;

HOOK_LISTENER(remote_line);
HOOK_LISTENER(client_accept);
HOOK_LISTENER(client_read);
HOOK_LISTENER(client_disconnect);
HOOK_LISTENER(client_line);

MODULE_INIT
{
	proxies = g_hash_table_new(NULL, NULL);
	proxyConnections = g_hash_table_new(NULL, NULL);
	clients = g_hash_table_new(NULL, NULL);

	HOOK_ATTACH(irc_line, remote_line);
	HOOK_ATTACH(socket_accept, client_accept);
	HOOK_ATTACH(socket_read, client_read);
	HOOK_ATTACH(socket_disconnect, client_disconnect);
	HOOK_ADD(irc_proxy_client_line);
	HOOK_ATTACH(irc_proxy_client_line, client_line);
	HOOK_ADD(irc_proxy_client_authenticated);

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

	HOOK_DEL(irc_proxy_client_authenticated);
	HOOK_DETACH(irc_proxy_client_line, client_line);
	HOOK_DEL(irc_proxy_client_line);
	HOOK_DETACH(irc_line, remote_line);
	HOOK_DETACH(socket_accept, client_accept);
	HOOK_DETACH(socket_read, client_read);
	HOOK_DETACH(socket_disconnect, client_disconnect);
}

HOOK_LISTENER(remote_line)
{
	IrcConnection *irc = HOOK_ARG(IrcConnection *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

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

HOOK_LISTENER(client_accept)
{
	Socket *server = HOOK_ARG(Socket *);
	Socket *client = HOOK_ARG(Socket *);

	IrcProxy *proxy;
	if((proxy = g_hash_table_lookup(proxies, server)) != NULL) { // One of our proxy servers accepted a new client
		$(bool, socket, enableSocketPolling)(client); // also poll the new socket

		LOG_INFO("New relay client %d for IRC proxy server on port %s", client->fd, proxy->server->port);

		IrcProxyClient *pc = ALLOCATE_OBJECT(IrcProxyClient);
		pc->proxy = proxy;
		pc->socket = client;
		pc->authenticated = false;
		pc->ibuffer = g_string_new("");

		g_hash_table_insert(clients, client, pc); // connect the client socket to the proxy client object
		g_queue_push_tail(proxy->clients, pc); // connect the proxy to its new client

		proxyClientIrcSend(pc, ":%s NOTICE AUTH :*** Welcome to the Kalisko IRC proxy server! Please use the PASS command to authenticate...", proxy->irc->socket->host);
	}
}

HOOK_LISTENER(client_read)
{
	Socket *socket = HOOK_ARG(Socket *);
	char *message = HOOK_ARG(char *);

	IrcProxyClient *client;
	if((client = g_hash_table_lookup(clients, socket)) != NULL) {
		g_string_append(client->ibuffer, message);
		checkForBufferLine(client);
	}
}

HOOK_LISTENER(client_disconnect)
{
	Socket *socket = HOOK_ARG(Socket *);

	IrcProxyClient *client;
	if((client = g_hash_table_lookup(clients, socket)) != NULL) { // one of our proxy clients disconnected
		LOG_INFO("IRC proxy client %d disconnected", client->socket->fd);
		g_queue_remove(client->proxy->clients, client); // remove the client from its irc proxy
		freeIrcProxyClient(client, "Bye");
	}
}

HOOK_LISTENER(client_line)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if(!client->authenticated) { // not yet authenticated, wait for password
		if(g_strcmp0(message->command, "PASS") == 0) {
			if(message->params_count > 0 && g_strcmp0(message->params[0], client->proxy->password) == 0) { // correct password
				LOG_INFO("IRC proxy client %d authenticated successfully", client->socket->fd);
				client->authenticated = true;
				proxyClientIrcSend(client, ":%s 001 %s :You were successfully authenticated and are now connected to the IRC server", client->proxy->irc->socket->host, client->proxy->irc->nick);
				proxyClientIrcSend(client, ":%s 251 %s :There are %d clients online on this bouncer", client->proxy->irc->socket->host, client->proxy->irc->nick, g_queue_get_length(client->proxy->clients));
				HOOK_TRIGGER(irc_proxy_client_authenticated, client);
			}
		}
	} else if(g_strcmp0(message->command, "PING") == 0) { // reply to pings
		if(message->trailing != NULL) {
			proxyClientIrcSend(client, "PONG :%s", message->trailing);
		}
	} else if(g_strcmp0(message->command, "USER") == 0) { // prevent user command from being passed through
		return;
	} else if(g_strcmp0(message->command, "QUIT") == 0) { // client disconnects
		LOG_DEBUG("IRC proxy client %d sent QUIT message, disconnecting...", client->socket->fd);
		$(bool, socket, disconnectSocket)(client->socket);
	} else {
		if((g_strcmp0(message->command, "PRIVMSG") == 0 || g_strcmp0(message->command, "NOTICE") == 0) && message->params_count > 0) { // potential filtered command
			for(GList *iter = client->proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
				if(g_strcmp0(iter->data, message->params[0]) == 0) { // target matches, so don't relay!
					return;
				}
			}
		}

		// Relay message to IRC server
		$(bool, irc, ircSend)(client->proxy->irc, "%s", message->raw_message);
	}
}

/**
 * Creates an IRC proxy relaying data for an IRC connection
 *
 * @param irc			the IRC connection to relay (should already be connected)
 * @param port			the server port to listen on for client connections
 * @param password		password to use for client connections
 * @result				the created IRC proxy, or NULL on failure
 */
API IrcProxy *createIrcProxy(IrcConnection *irc, char *port, char *password)
{
	Socket *server = $(Socket *, socket, createServerSocket)(port);

	if(!$(bool, socket, connectSocket)(server)) {
		return NULL;
	}

	if(!$(bool, socket, enableSocketPolling)(server)) {
		return NULL;
	}

	IrcProxy *proxy = ALLOCATE_OBJECT(IrcProxy);
	proxy->irc = irc;
	proxy->server = server;
	proxy->password = strdup(password);
	proxy->clients = g_queue_new();
	proxy->relay_exceptions = g_queue_new();

	g_hash_table_insert(proxies, server, proxy);
	g_hash_table_insert(proxyConnections, irc, proxy);

	return proxy;
}

/**
 * Frees an IRC proxy. Note that this doesn't disconnect or free the used IRC connection
 *
 * @param proxy			the IRC proxy to free
 * @result				true if successful
 */
API void freeIrcProxy(IrcProxy *proxy)
{
	g_hash_table_remove(proxies, proxy->server);
	g_hash_table_remove(proxyConnections, proxy->irc);

	// Free relay exceptions
	for(GList *iter = proxy->relay_exceptions->head; iter != NULL; iter = iter->next) {
		free(iter->data); // free the exception target
	}

	g_queue_free(proxy->relay_exceptions); // free the exceptions list

	$(bool, socket, freeSocket)(proxy->server);
	g_queue_foreach(proxy->clients, &freeIrcProxyClient, "IRC proxy server going down");
	g_queue_free(proxy->clients);
	free(proxy->password);
	free(proxy);
}

/**
 * Adds a relay exception to an IRC proxy. NOTICE and PRIVMSG messages to this target will not be relayed to the remote IRC connection.
 * Use this to implement virtual bots for custom modules in your bouncer
 *
 * @param proxy			the IRC proxy to add the exception for
 * @param exception		the target to which no messages should be relayed
 */
API void addIrcProxyRelayException(IrcProxy *proxy, char *exception)
{
	g_queue_push_tail(proxy->relay_exceptions, strdup(exception));
}

/**
 * Removes a relay exception to an IRC proxy.
 * @see addIrcProxyRelayException
 *
 * @param proxy			the IRC proxy to remove the exception for
 * @param exception		the target to which messages should be relayed again
 * @result				true if successful
 */
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

/**
 * Sends a message to an IRC client socket
 *
 * @param client		the socket of the IRC client
 * @param message		frintf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool proxyClientIrcSend(IrcProxyClient *client, char *message, ...)
{
	va_list va;
	char buffer[IRC_SEND_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, IRC_SEND_MAXLEN, message, va);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = $(bool, socket, socketWriteRaw)(client->socket, nlmessage->str, nlmessage->len);

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

	g_hash_table_remove(clients, client->socket); // remove ourselves from the irc proxy client sockets table

	$(void, socket, freeSocket)(client->socket); // free the socket
	g_string_free(client->ibuffer, true); // free the input buffer

	free(client); // actually free the client
}

/**
 * Checks for a new newline terminated line in the buffer and parses it
 *
 * @param client			the IRC proxz client to check for buffer lines
 */
static void checkForBufferLine(IrcProxyClient *client)
{
	char *message = client->ibuffer->str;

	if(strstr(message, "\n") != NULL) { // message has at least one newline
		g_string_free(client->ibuffer, false);
		$(void, string_util, stripDuplicateNewlines)(message); // remove duplicate newlines, since server could send \r\n
		char **parts = g_strsplit(message, "\n", 0);

		char **iter;
		for(iter = parts; *iter != NULL; iter++) {
			if(*(iter + 1) != NULL) { // Don't trigger the last part, it's not yet complete
				if(strlen(*iter) > 0) {
					IrcMessage *ircMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(*iter);

					if(ircMessage != NULL) {
						HOOK_TRIGGER(irc_proxy_client_line, client, ircMessage);
						$(void, irc_parser, freeIrcMessage)(ircMessage);
					}
				}
			}
		}

		client->ibuffer = g_string_new(*(iter - 1)); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
