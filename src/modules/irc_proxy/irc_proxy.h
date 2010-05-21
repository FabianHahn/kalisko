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


#ifndef IRC_PROXY_IRC_PROXY_H
#define IRC_PROXY_IRC_PROXY_H

#include <glib.h>
#include "modules/socket/socket.h"
#include "modules/irc/irc.h"

/**
 * Struct to represent an IRC proxy
 */
typedef struct {
		/** the global unique proxy ID of this proxy */
		int id;
		/** the IRC connection that should be relayed to clients */
		IrcConnection *irc;
		/** the proxy password for connecting clients */
		char *password;
		/** list of client sockets to and from which we relay */
		GQueue *clients;
		/** list of PRIVMSG or NOTICE targets that should not be relayed to the remote IRC connection */
		GQueue *relay_exceptions;
} IrcProxy;

/**
 * Struct to represent the client of an IRC proxy
 */
typedef struct {
		/** the client's IRC proxy */
		IrcProxy *proxy;
		/** the socket for the client connection */
		Socket *socket;
		/** true if the client passed the password challenge */
		bool authenticated;
		/** the line input buffer for the client */
		GString *ibuffer;
} IrcProxyClient;

API IrcProxy *createIrcProxy(int id, IrcConnection *irc, char *password);
API IrcProxy *getIrcProxyByIrcConnection(IrcConnection *irc);
API IrcProxy *getIrcProxyById(int id);
API void freeIrcProxy(IrcProxy *proxy);
API void addIrcProxyRelayException(IrcProxy *proxy, char *exception);
API bool delIrcProxyRelayException(IrcProxy *proxy, char *exception);
API bool proxyClientIrcSend(IrcProxyClient *client, char *message, ...) G_GNUC_PRINTF(2, 3);

#endif
