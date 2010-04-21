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

#include <stdio.h> // vsnprintf
#include <stdarg.h> // va_list, va_start
#include <string.h>
#include <glib.h>
#include "dll.h"
#include "log.h"
#include "hooks.h"
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/string_util/string_util.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"
#include "irc.h"

MODULE_NAME("irc");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module connects to an IRC server and does basic communication to keep the connection alive");
MODULE_VERSION(0, 2, 3);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("socket", 0, 3, 0), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0));

HOOK_LISTENER(irc_line);
HOOK_LISTENER(irc_read);

static GHashTable *connections;

static void checkForBufferLine(IrcConnection *irc);

MODULE_INIT
{
	connections = g_hash_table_new(NULL, NULL);

	HOOK_ATTACH(socket_read, irc_read);
	HOOK_ADD(irc_send);
	HOOK_ADD(irc_line);
	HOOK_ATTACH(irc_line, irc_line);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(connections);

	HOOK_DEL(irc_send);
	HOOK_DETACH(irc_line, irc_line);
	HOOK_DEL(irc_line);
	HOOK_DETACH(socket_read, irc_read);
}

HOOK_LISTENER(irc_read)
{
	Socket *socket = HOOK_ARG(Socket *);
	char *message = HOOK_ARG(char *);

	IrcConnection *irc;
	if((irc = g_hash_table_lookup(connections, socket)) != NULL) {
		g_string_append(irc->ibuffer, message);
		checkForBufferLine(irc);
	}
}

HOOK_LISTENER(irc_line)
{
	IrcConnection *irc = HOOK_ARG(IrcConnection *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if(g_strcmp0(message->command, "PING") == 0) {
		char *challenge = message->trailing;
		ircSend(irc, "PONG :%s", challenge);
	}
}

/**
 * Creates an IRC connection
 *
 * @param server		IRC server to connect to
 * @param port			IRC server's port to connect to
 * @param user			user name to use
 * @param real			real name to use
 * @param nick			nick to use
 * @result				the created IRC connection or NULL on failure
 */
API IrcConnection *createIrcConnection(char *server, char *port, char *password, char *user, char *real, char *nick)
{
	IrcConnection *irc = ALLOCATE_OBJECT(IrcConnection);
	irc->user = strdup(user);
	irc->password = strdup(password);
	irc->real = strdup(real);
	irc->nick = strdup(nick);
	irc->ibuffer = g_string_new("");
	irc->socket = $(Socket *, socket, createClientSocket)(server, port);

	if(!$(bool, socket, connectSocket)(irc->socket) || !$(bool, socket, enableSocketPolling)(irc->socket)) {
		LOG_ERROR("Failed to connect IRC connection socket");
		$(void, socket, freeIrcConnection)(irc);
		return NULL;
	}

	ircSend(irc, "USER %s 0 0 :%s", user, real);
	ircSend(irc, "NICK %s", nick);

	g_hash_table_insert(connections, irc->socket, irc);

	return irc;
}

/**
 * Creates an IRC connection by reading the required parameters from a store
 *
 * @param params		the store to read the parameters from
 * @result				the created IRC connection or NULL on failure
 */
API IrcConnection *createIrcConnectionByStore(Store *params)
{
	Store *param;
	char *server;
	char *port;
	char *password;
	char *user;
	char *real;
	char *nick;

	if((param = $(Store *, config, getStorePath)(params, "server")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'server', aborting IRC connection");
		return NULL;
	}

	server = param->content.string;

	if((param = $(Store *, config, getStorePath)(params, "port")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'port', aborting IRC connection");
		return NULL;
	}

	port = param->content.string;

	if((param = $(Store *, config, getStorePath)(params, "password")) != NULL && param->type == STORE_STRING) {
		password = param->content.string;
	} else {
		password = NULL;
	}

	if((param = $(Store *, config, getStorePath)(params, "user")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'user', aborting IRC connection");
		return NULL;
	}

	user = param->content.string;

	if((param = $(Store *, config, getStorePath)(params, "real")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'real', aborting IRC connection");
		return NULL;
	}

	real = param->content.string;

	if((param = $(Store *, config, getStorePath)(params, "nick")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'nick', aborting IRC connection");
		return NULL;
	}

	nick = param->content.string;

	return createIrcConnection(server, port, password, user, real, nick);
}

/**
 * Frees an IRC connection
 *
 * @param irc		the IRC connection to free
 */
API void freeIrcConnection(IrcConnection *irc)
{
	g_hash_table_remove(connections, irc->socket);

	$(bool, socket, freeSocket)(irc->socket);
	free(irc->password);
	free(irc->user);
	free(irc->real);
	free(irc->nick);
	g_string_free(irc->ibuffer, true);

	// Last, free the irc connection object itself
	free(irc);
}

/**
 * Sends a message to the IRC socket
 *
 * @param irc			the IRC connection to use for sending the message
 * @param message		frintf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool ircSend(IrcConnection *irc, char *message, ...)
{
	va_list va;
	char buffer[IRC_SEND_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, IRC_SEND_MAXLEN, message, va);

	HOOK_TRIGGER(irc_send, irc, buffer);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = $(bool, socket, socketWriteRaw)(irc->socket, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

/**
 * Checks for a new newline terminated line in the buffer and parses it
 *
 * @param irc			the IRC connection to check for buffer lines
 */
static void checkForBufferLine(IrcConnection *irc)
{
	char *message = irc->ibuffer->str;

	if(strstr(message, "\n") != NULL) { // message has at least one newline
		g_string_free(irc->ibuffer, false);
		$(void, string_util, stripDuplicateNewlines)(message); // remove duplicate newlines, since server could send \r\n
		char **parts = g_strsplit(message, "\n", 0);

		char **iter;
		for(iter = parts; *iter != NULL; iter++) {
			if(*(iter + 1) != NULL) { // Don't trigger the last part, it's not yet complete
				if(strlen(*iter) > 0) {
					IrcMessage *ircMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(*iter);

					if(ircMessage != NULL) {
						HOOK_TRIGGER(irc_line, irc, ircMessage);
					}

					$(void, irc_parser, freeIrcMessage)(ircMessage);
				}
			}
		}

		irc->ibuffer = g_string_new(*(iter - 1)); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
