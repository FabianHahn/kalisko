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
		/** the global unique proxy name of this proxy */
		char *name;
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


/**
 * Creates an IRC proxy relaying data for an IRC connection
 *
 * @param name			the global unique name to use for this IRC proxy
 * @param irc			the IRC connection to relay (should already be connected)
 * @param password		password to use for client connections
 * @result				the created IRC proxy, or NULL on failure
 */
API IrcProxy *createIrcProxy(char *name, IrcConnection *irc, char *password);

/**
 * Returns a list of all IRC proxies created
 *
 * @result		a list of all created IRC proxies, must not be modified but freed with g_list_free after use
 */
API GList *getIrcProxies();

/**
 * Retrieves an IRC proxy by its remote IRC connection
 *
 * @param irc	the IRC connection to lookup
 * @result		the IRC proxy or NULL if no proxy is enabled for this connection
 */
API IrcProxy *getIrcProxyByIrcConnection(IrcConnection *irc);

/**
 * Retrieves an IRC proxy by its global unique name
 *
 * @param name		the name to look up
 * @result			the IRC proxy or NULL if not found
 */
API IrcProxy *getIrcProxyByName(char *name);

/**
 * Retrieves an IRC proxy client by its socket
 *
 * @param socket		the socket to look up
 * @result				the IRC proxy client, or NULL if none was found for this socket
 */
API IrcProxyClient *getIrcProxyClientBySocket(Socket *socket);

/**
 * Frees an IRC proxy. Note that this doesn't disconnect or free the used IRC connection
 *
 * @param proxy			the IRC proxy to free
 * @result				true if successful
 */
API void freeIrcProxy(IrcProxy *proxy);

/**
 * Adds a relay exception to an IRC proxy. NOTICE and PRIVMSG messages to this target will not be relayed to the remote IRC connection.
 * Use this to implement virtual bots for custom modules in your bouncer
 *
 * @param proxy			the IRC proxy to add the exception for
 * @param exception		the target to which no messages should be relayed
 */
API void addIrcProxyRelayException(IrcProxy *proxy, char *exception);

/**
 * Removes a relay exception to an IRC proxy.
 * @see addIrcProxyRelayException
 *
 * @param proxy			the IRC proxy to remove the exception for
 * @param exception		the target to which messages should be relayed again
 * @result				true if successful
 */
API bool delIrcProxyRelayException(IrcProxy *proxy, char *exception);

/**
 * Checks if a proxy has a certain relay exception
 * @see addIrcProxyRelayException
 *
 * @param proxy			the IRC proxy to check for an exception
 * @param exception		the target that should be checked
 * @result				true if the parameter is a relay exception
 */
API bool hasIrcProxyRelayException(IrcProxy *proxy, char *exception);

/**
 * Sends a message to an IRC client socket
 *
 * @param client		the socket of the IRC client
 * @param message		printf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool proxyClientIrcSend(IrcProxyClient *client, char *message, ...) G_GNUC_PRINTF(2, 3);

#endif
