/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
MODULE_VERSION(0, 5, 2);
MODULE_BCVERSION(0, 5, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("socket", 0, 7, 0), MODULE_DEPENDENCY("string_util", 0, 1, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_throttlePoll(void *subject, const char *event, void *data, va_list args);
static void listener_ircConnected(void *subject, const char *event, void *data, va_list args);
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

	attachEventListener(NULL, "sockets_polled", NULL, &listener_throttlePoll);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(connections);
	g_queue_free(throttled);

	detachEventListener(NULL, "sockets_polled", NULL, &listener_throttlePoll);
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
			triggerEvent(irc, "send", line->str); // trigger send event
			g_string_append_c(line, '\n'); // append newline
			socketWriteRaw(irc->socket, line->str, line->len); // send it
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

static void listener_ircConnected(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;

	IrcConnection *irc;
	if((irc = g_hash_table_lookup(connections, socket)) != NULL) { // This socket belongs to an IRC connection
		// Reauthenticate the connection
		authenticateIrcConnection(irc);
	}
}

static void listener_ircDisconnect(void *subject, const char *event, void *data, va_list args)
{
	Socket *socket = subject;

	IrcConnection *irc;
	if((irc = g_hash_table_lookup(connections, socket)) != NULL) { // This socket belongs to an IRC connection
		triggerEvent(irc, "disconnect");
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
		ircSendFirst(irc, "PONG :%s", challenge);
		triggerEvent(irc, "pinged");
	} else if(g_strcmp0(message->command, "251") == 0) { // registered with server
		if(message->params_count > 0 && message->params != NULL && message->params[0] != NULL) { // change the nick to whatever the server registered us with
			if(g_strcmp0(irc->nick, message->params[0]) != 0) {
				free(irc->nick);
				irc->nick = strdup(message->params[0]);
				triggerEvent(irc, "nick"); // notify listeners
			}
		}

		triggerEvent(irc, "reconnect");
	} else if(g_strcmp0(message->command, "NICK") == 0) {
		IrcUserMask *mask = parseIrcUserMask(message->prefix);
		if(mask != NULL) {
			if(g_strcmp0(irc->nick, mask->nick) == 0) { // Our own nickname was changed
				free(irc->nick); // free old nick
				irc->nick = strdup(mask->nick); // store new nick
				triggerEvent(irc, "nick"); // notify listeners
			}

			free(mask);
		}
	}
}

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
	irc->socket = createClientSocket(server, port);

	attachEventListener(irc->socket, "connected", NULL, &listener_ircConnected);
	attachEventListener(irc->socket, "read", NULL, &listener_ircRead);
	attachEventListener(irc->socket, "disconnect", NULL, &listener_ircDisconnect);
	attachEventListener(irc, "line", NULL, &listener_ircLine);

	if(!connectClientSocketAsync(irc->socket, 10)) {
		logError("Failed to connect IRC connection socket");
		freeIrcConnection(irc);
		return NULL;
	}

	g_hash_table_insert(connections, irc->socket, irc);

	return irc;
}

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

	if((param = getStorePath(params, "server")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required params value 'server', aborting IRC connection");
		return NULL;
	}

	server = param->content.string;

	if((param = getStorePath(params, "port")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required params value 'port', aborting IRC connection");
		return NULL;
	}

	port = param->content.string;

	if((param = getStorePath(params, "password")) != NULL && param->type == STORE_STRING) {
		password = param->content.string;
	} else {
		password = NULL;
	}

	if((param = getStorePath(params, "user")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required params value 'user', aborting IRC connection");
		return NULL;
	}

	user = param->content.string;

	if((param = getStorePath(params, "real")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required params value 'real', aborting IRC connection");
		return NULL;
	}

	real = param->content.string;

	if((param = getStorePath(params, "nick")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required params value 'nick', aborting IRC connection");
		return NULL;
	}

	nick = param->content.string;

	if((param = getStorePath(params, "throttle")) == NULL || param->type != STORE_INTEGER) {
		logError("Could not find required params value 'throttle', aborting IRC connection");
		return NULL;
	}

	throttle = param->content.integer;

	IrcConnection *connection = createIrcConnection(server, port, password, user, real, nick);

	if(throttle > 0 && connection != NULL) {
		enableIrcConnectionThrottle(connection);
	}

	return connection;
}

API bool reconnectIrcConnection(IrcConnection *irc)
{
	if(!irc->socket->connected) { // note that we can't check whether the socket is active because this could be called right before the socket is removed from the polling queue...
		if(connectClientSocketAsync(irc->socket, 10)) {
			return true;
		}
	} else {
		logError("Cannot reconnect already connected IRC connection with socket %d", irc->socket->fd);
	}

	return false;
}

API bool enableIrcConnectionThrottle(IrcConnection *irc)
{
	if(irc->throttle) {
		logWarning("Trying to enable throttling on already enabled IRC connection with socket %d", irc->socket->fd);
		return false;
	}

	irc->throttle = true;
	irc->obuffer = g_queue_new();
	irc->throttle_time = $$(double, getMicroTime)();

	g_queue_push_tail(throttled, irc);

	logNotice("Enabled throttling for IRC connection with socket %d", irc->socket->fd);

	return true;
}

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

API void freeIrcConnection(IrcConnection *irc)
{
	if(irc->throttle) {
		disableIrcConnectionThrottle(irc, false);
	}

	g_hash_table_remove(connections, irc->socket);

	detachEventListener(irc->socket, "read", NULL, &listener_ircRead);
	detachEventListener(irc->socket, "connected", NULL, &listener_ircConnected);
	detachEventListener(irc->socket, "disconnect", NULL, &listener_ircDisconnect);
	detachEventListener(irc, "line", NULL, &listener_ircLine);

	freeSocket(irc->socket);
	free(irc->password);
	free(irc->user);
	free(irc->real);
	free(irc->nick);
	g_string_free(irc->ibuffer, true);

	// Last, free the irc connection object itself
	free(irc);
}

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

	triggerEvent(irc, "send", buffer);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = socketWriteRaw(irc->socket, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

API bool ircSendFirst(IrcConnection *irc, char *message, ...)
{
	va_list va;
	char buffer[IRC_SEND_MAXLEN];

	va_start(va, message);
	vsnprintf(buffer, IRC_SEND_MAXLEN, message, va);

	if(irc->throttle) { // delay message
		g_queue_push_head(irc->obuffer, g_string_new(buffer));
		return true;
	}

	triggerEvent(irc, "send", buffer);

	GString *nlmessage = g_string_new(buffer);
	g_string_append_c(nlmessage, '\n');

	bool ret = socketWriteRaw(irc->socket, nlmessage->str, nlmessage->len);

	g_string_free(nlmessage, true);

	return ret;
}

API void authenticateIrcConnection(IrcConnection *irc)
{
	if(irc->password != NULL) {
		ircSend(irc, "PASS %s", irc->password);
	}

	ircSend(irc, "USER %s 0 0 :%s", irc->user, irc->real);
	ircSend(irc, "NICK %s", irc->nick);
}

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
					triggerEvent(irc, "line", ircMessage);
					freeIrcMessage(ircMessage);
				}
			}
		}

		irc->ibuffer = g_string_new(parts[count-1]); // reinitialize buffer

		g_strfreev(parts);
		free(message);
	}
}
