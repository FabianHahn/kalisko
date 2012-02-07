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
#include "memory_alloc.h"
#include "util.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/string_util/string_util.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/event/event.h"
#define API
#include "irc.h"

MODULE_NAME("irc");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module connects to an IRC server and does basic communication to keep the connection alive");
MODULE_VERSION(0, 4, 7);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("socket", 0, 4, 3), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_throttlePoll(void *subject, const char *event, void *data, va_list args);
static void listener_ircDisconnect(void *subject, const char *event, void *data, va_list args);
static void listener_ircLine(void *subject, const char *event, void *data, va_list args);
static void listener_ircRead(void *subject, const char *event, void *data, va_list args);

static GHashTable *connections;
static GQueue *throttled;

static void checkForBufferLine(IrcConnection *irc);

MODULE_INIT
{
	connections = g_hash_table_new(NULL, NULL);
	throttled = g_queue_new();

	$(void, event, attachEventListener)(NULL, "sockets_polled", NULL, &listener_throttlePoll);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(connections);
	g_queue_free(throttled);

	$(void, event, detachEventListener)(NULL, "sockets_polled", NULL, &listener_throttlePoll);
}

static void listener_throttlePoll(void *subject, const char *event, void *data, va_list args)
{
	double now = $$(double, getMicroTime)();

	GQueue *dead = g_queue_new();

	for(GList *iter = throttled->head; iter != NULL; iter = iter->next) { // loop over all throttled connections
		IrcConnection *irc = iter->data;

		if(!irc->socket->connected) { // Check if IRC connection is still alive
			g_queue_push_head(dead, irc); // If not, add to dead list and continue
			continue;
		}

		if(now > irc->throttle_time) {
			irc->throttle_time = now;
		}

		while(!g_queue_is_empty(irc->obuffer) && (irc->throttle_time - now) < 10.0) { // repeat while lines and throttle space left
			GString *line = g_queue_pop_head(irc->obuffer); // pop the next output buffer line
			$(int, event, triggerEvent)(irc, "send", line->str); // trigger send event
			g_string_append_c(line, '\n'); // append newline
			$(bool, socket, socketWriteRaw)(irc->socket, line->str, line->len); // send it
			irc->throttle_time += (2.0 + line->len) / 120.0; // penalty throttle time by line length
			g_string_free(line, true); // free it
		}
	}

	// Now cleanup all dead sockets
	for(GList *iter = dead->head; iter != NULL; iter = iter->next) { // loop over all dead sockets
		IrcConnection *irc = iter->data;
		disableIrcConnectionThrottle(irc, false);
	}

	g_queue_free(dead);
}

static void listener_ircDisconnect(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;

	IrcConnection *irc;
	if((irc = g_hash_table_lookup(connections, socket)) != NULL) { // This socket belongs to an IRC connection
		$(int, event, triggerEvent)(irc, "disconnect");
	}
}

static void listener_ircRead(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;
	char *message = va_arg(args, char *);

	IrcConnection *irc;
	if((irc = g_hash_table_lookup(connections, socket)) != NULL) {
		g_string_append(irc->ibuffer, message);
		checkForBufferLine(irc);
	}
}

static void listener_ircLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if(g_strcmp0(message->command, "PING") == 0) {
		char *challenge = message->trailing;
		ircSend(irc, "PONG :%s", challenge);
	} else if(g_strcmp0(message->command, "NICK") == 0) {
		IrcUserMask *mask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix);
		if(mask != NULL) {
			if(g_strcmp0(irc->nick, mask->nick) == 0) { // Our own nickname was changed
				free(irc->nick); // free old nick
				irc->nick = strdup(mask->nick); // store new nick
				$(int, event, triggerEvent)(irc, "nick"); // notify listeners
			}

			free(mask);
		}
	}
}

/**
 * Creates an IRC connection
 *
 * @param server		IRC server to connect to
 * @param port			IRC server's port to connect to
 * @param user			user name to use
 * @param password		password to use
 * @param real			real name to use
 * @param nick			nick to use
 * @result				the created IRC connection or NULL on failure
 */
API IrcConnection *createIrcConnection(char *server, char *port, char *password, char *user, char *real, char *nick)
{
	IrcConnection *irc = ALLOCATE_OBJECT(IrcConnection);
	irc->user = strdup(user);
	if(password != NULL) {
		irc->password = strdup(password);
	} else {
		irc->password = NULL;
	}
	irc->real = strdup(real);
	irc->nick = strdup(nick);
	irc->ibuffer = g_string_new("");
	irc->throttle = false;
	irc->obuffer = NULL;
	irc->socket = $(Socket *, socket, createClientSocket)(server, port);

	$(void, event, attachEventListener)(irc->socket, "read", NULL, &listener_ircRead);
	$(void, event, attachEventListener)(irc->socket, "disconnect", NULL, &listener_ircDisconnect);
	$(void, event, attachEventListener)(irc, "line", NULL, &listener_ircLine);

	if(!$(bool, socket, connectSocket)(irc->socket) || !$(bool, socket, enableSocketPolling)(irc->socket)) {
		LOG_ERROR("Failed to connect IRC connection socket");
		freeIrcConnection(irc);
		return NULL;
	}

	authenticateIrcConnection(irc);

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
	int throttle;

	if((param = $(Store *, store, getStorePath)(params, "server")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'server', aborting IRC connection");
		return NULL;
	}

	server = param->content.string;

	if((param = $(Store *, store, getStorePath)(params, "port")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'port', aborting IRC connection");
		return NULL;
	}

	port = param->content.string;

	if((param = $(Store *, store, getStorePath)(params, "password")) != NULL && param->type == STORE_STRING) {
		password = param->content.string;
	} else {
		password = NULL;
	}

	if((param = $(Store *, store, getStorePath)(params, "user")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'user', aborting IRC connection");
		return NULL;
	}

	user = param->content.string;

	if((param = $(Store *, store, getStorePath)(params, "real")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'real', aborting IRC connection");
		return NULL;
	}

	real = param->content.string;

	if((param = $(Store *, store, getStorePath)(params, "nick")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'nick', aborting IRC connection");
		return NULL;
	}

	nick = param->content.string;

	if((param = $(Store *, store, getStorePath)(params, "throttle")) == NULL || param->type != STORE_INTEGER) {
		LOG_ERROR("Could not find required params value 'throttle', aborting IRC connection");
		return NULL;
	}

	throttle = param->content.integer;

	IrcConnection *connection = createIrcConnection(server, port, password, user, real, nick);

	if(throttle > 0 && connection != NULL) {
		enableIrcConnectionThrottle(connection);
	}

	return connection;
}

/**
 * Enables output throttling for an IRC connection
 *
 * @param irc		the IRC connection to enable output throttling for
 * @result			true if successful
 */
API bool enableIrcConnectionThrottle(IrcConnection *irc)
{
	if(irc->throttle) {
		LOG_WARNING("Trying to enable throttling on already enabled IRC connection with socket %d", irc->socket->fd);
		return false;
	}

	irc->throttle = true;
	irc->obuffer = g_queue_new();
	irc->throttle_time = $$(double, getMicroTime)();

	g_queue_push_tail(throttled, irc);

	LOG_INFO("Enabled throttling for IRC connection with socket %d", irc->socket->fd);

	return true;
}

/**
 * Disables output throttling for an IRC connection
 *
 * @param irc					the IRC connection to disable output throttling for
 * @param flush_output_buffer	if true, the output buffer is flushed before freeing, i.e. all remaining buffered messages will be burst-sent to the server
 */
API void disableIrcConnectionThrottle(IrcConnection *irc, bool flush_output_buffer)
{
	if(!irc->throttle) {
		return;
	}

	irc->throttle = false; // disable throttling before flushing

	for(GList *iter = irc->obuffer->head; iter != NULL; iter = iter->next) {
		GString *line = iter->data;

		if(flush_output_buffer && irc->socket->connected) { // if flushing and still connected
			ircSend(irc, "%s", line->str);
		}

		g_string_free(line, true);
	}

	g_queue_free(irc->obuffer); // free output buffer
	g_queue_remove(throttled, irc); // remove ourselves from output buffer list
}

/**
 * Frees an IRC connection
 *
 * @param irc		the IRC connection to free
 */
API void freeIrcConnection(IrcConnection *irc)
{
	if(irc->throttle) {
		disableIrcConnectionThrottle(irc, false);
	}

	g_hash_table_remove(connections, irc->socket);

	$(void, event, detachEventListener)(irc->socket, "read", NULL, &listener_ircRead);
	$(void, event, detachEventListener)(irc->socket, "disconnect", NULL, &listener_ircDisconnect);
	$(void, event, detachEventListener)(irc, "line", NULL, &listener_ircLine);

	$(void, socket, freeSocket)(irc->socket);
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

	if(irc->throttle) { // delay message
		g_queue_push_tail(irc->obuffer, g_string_new(buffer));
		return true;
	}

	$(int, event, triggerEvent)(irc, "send", buffer);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = $(bool, socket, socketWriteRaw)(irc->socket, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

/**
 * Authenticate an IRC connection by sending USER, NICK and PASS lines
 *
 * @param irc		the IRC connection to authenticate
 */
API void authenticateIrcConnection(IrcConnection *irc)
{
	if(irc->password != NULL) {
		ircSend(irc, "PASS %s", irc->password);
	}

	ircSend(irc, "USER %s 0 0 :%s", irc->user, irc->real);
	ircSend(irc, "NICK %s", irc->nick);
}

/**
 * Retrieves an IRC connection by its socket
 *
 * @param socket		the socket to look up
 * @result				the IRC connection, or NULL if none was found for this socket
 */
API IrcConnection *getIrcConnectionBySocket(Socket *socket)
{
	return g_hash_table_lookup(connections, socket);
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
						$(int, event, triggerEvent)(irc, "line", ircMessage);

						// Free the IRC message
						$(void, irc_parser, freeIrcMessage)(ircMessage);
					}
				}
			}
		}

		irc->ibuffer = g_string_new(*(iter - 1)); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
