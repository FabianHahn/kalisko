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
#include "modules/config/config.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/string_util/string_util.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"
#include "irc.h"

MODULE_NAME("irc");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module connects to an IRC server and does basic communication to keep the connection alive");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 2, 0), MODULE_DEPENDENCY("socket", 0, 2, 2), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0));

HOOK_LISTENER(irc_line);
HOOK_LISTENER(irc_read);
HOOK_LISTENER(irc_disconnect);

static Socket *sock;
static GString *buffer;
static char *nick;

static void checkForBufferLine();

MODULE_INIT
{
	StoreNodeValue *config;
	char *server;
	char *port;
	char *user;
	char *real;

	buffer = g_string_new("");

	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/server")) == NULL || config->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'irc/server', aborting");
		return false;
	}

	server = config->content.string;

	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/port")) == NULL || config->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'irc/port', aborting");
		return false;
	}

	port = config->content.string;

	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/user")) == NULL || config->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'irc/user', aborting");
		return false;
	}

	user = config->content.string;

	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/real")) == NULL || config->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'irc/real', aborting");
		return false;
	}

	real = config->content.string;

	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/nick")) == NULL || config->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'irc/nick', aborting");
		return false;
	}

	nick = config->content.string;

	sock = $(Socket *, socket, createClientSocket)(server, port);

	if(!$(bool, socket, connectSocket)(sock) || !$(bool, socket, enableSocketPolling)(sock)) {
		LOG_ERROR("Failed to connect IRC socket");
		$(void, socket, freeSocket)(sock);
	}

	ircSend("USER %s 0 0 :%s", user, real);
	ircSend("NICK %s", nick);
	
	if((config = $(StoreNodeValue *, config, getConfigPathValue)("irc/serverpass")) != NULL && config->type == STORE_STRING) {
		ircSend("PASS %s", config->content.string);
	}

	HOOK_ATTACH(socket_read, irc_read);
	HOOK_ATTACH(socket_disconnect, irc_disconnect);
	HOOK_ADD(irc_send);
	HOOK_ADD(irc_line);
	HOOK_ATTACH(irc_line, irc_line);

	return true;
}

MODULE_FINALIZE
{
	ircSend("QUIT :Kalisko module unload :(");

	HOOK_DEL(irc_send);
	HOOK_DETACH(irc_line, irc_line);
	HOOK_DEL(irc_line);
	HOOK_DETACH(socket_read, irc_read);
	HOOK_DETACH(socket_disconnect, irc_disconnect);
	$(void, socket, freeSocket)(sock);
}

HOOK_LISTENER(irc_read)
{
	Socket *readSocket = HOOK_ARG(Socket *);
	char *message = HOOK_ARG(char *);

	if(readSocket == sock) {
		g_string_append(buffer, message);
		checkForBufferLine();
	}
}

HOOK_LISTENER(irc_disconnect)
{
	Socket *readSocket = HOOK_ARG(Socket *);

	if(readSocket == sock) {
		LOG_WARNING("IRC socket disconnected!");
	}
}

HOOK_LISTENER(irc_line)
{
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if(g_strcmp0(message->command, "PING") == 0) {
		char *challenge = message->trailing;
		ircSend("PONG :%s", challenge);
	}
}

API char *getNick()
{
	return nick;
}

/**
 * Sens a message to the IRC socket
 *
 * @param message		frintf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool ircSend(char *message, ...)
{
	va_list va;
	char buffer[IRC_SEND_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, IRC_SEND_MAXLEN, message, va);

	HOOK_TRIGGER(irc_send, buffer);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = $(bool, socket, socketWriteRaw)(sock, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

/**
 * Checks for a new newline terminated line in the buffer and parses it
 */
static void checkForBufferLine()
{
	char *message = buffer->str;

	if(strstr(message, "\n") != NULL) { // message has at least one newline
		g_string_free(buffer, FALSE);
		$(void, string_util, stripDuplicateNewlines)(message); // remove duplicate newlines, since server could send \r\n
		char **parts = g_strsplit(message, "\n", 0);

		char **iter;
		for(iter = parts; *iter != NULL; iter++) {
			if(*(iter + 1) != NULL) { // Don't trigger the last part, it's not yet complete
				if(strlen(*iter) > 0) {
					IrcMessage *ircMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(*iter);

					if(ircMessage != NULL) {
						HOOK_TRIGGER(irc_line, ircMessage);
					}

					$(void, irc_parser, freeIrcMessage)(ircMessage);
				}
			}
		}

		buffer = g_string_new(*(iter - 1)); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
