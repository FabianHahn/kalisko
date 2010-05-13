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
#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "modules/config/config.h"
#include "modules/store/path.h"
#include "modules/irc/irc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "api.h"

MODULE_NAME("ircpp_keepalive");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that tries to keep the connection to the remote IRC server alive by pinging it in regular intervals");
MODULE_VERSION(0, 3, 2);
MODULE_BCVERSION(0, 3, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 0), MODULE_DEPENDENCY("socket", 0, 4, 4), MODULE_DEPENDENCY("irc", 0, 4, 1), MODULE_DEPENDENCY("irc_proxy", 0, 1, 15), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 1), MODULE_DEPENDENCY("store", 0, 5, 3));

TIMER_CALLBACK(challenge);
TIMER_CALLBACK(challenge_timeout);
HOOK_LISTENER(remote_line);
HOOK_LISTENER(remote_disconnect);
static void reconnectRemoteConnection(IrcProxy *proxy);
static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);
static IrcProxyPlugin plugin;

/**
 * Hash table to associate IrcProxy objects with the GTimeVal objects for the keepalive challenges
 */
static GHashTable *challenges;

/**
 * Hash table to associate IrcProxy objects with the GTimeVal objects for the keepalive challenge timeouts
 */
static GHashTable *challengeTimeouts;

/**
 * Interval between keepalive challenges
 */
static unsigned int keepaliveInterval = 60 * G_USEC_PER_SEC;

/**
 * Timeout until the remote IRC connection has to send a PONG response to a challenge
 */
static unsigned int keepaliveTimeout = 5 * G_USEC_PER_SEC;


MODULE_INIT
{
	// Check if config values are set
	Store *configKeepaliveInterval = $(Store *, store, getStorePath)($(Store *, config, getConfig)(), "irc/keepalive/interval");
	if(configKeepaliveInterval != NULL && configKeepaliveInterval->type == STORE_INTEGER) {
		keepaliveInterval = configKeepaliveInterval->content.integer;
	} else {
		LOG_INFO("Could not determine config value irc/keepalive/interval, using default value of %d", keepaliveInterval);
	}

	Store *configKeepaliveTimeout = $(Store *, store, getStorePath)($(Store *, config, getConfig)(), "irc/keepalive/timeout");
	if(configKeepaliveTimeout != NULL && configKeepaliveTimeout->type == STORE_INTEGER) {
		keepaliveTimeout = configKeepaliveTimeout->content.integer;
	} else {
		LOG_INFO("Could not determine config value irc/keepalive/timeout, using default value of %d", keepaliveTimeout);
	}

	challenges = g_hash_table_new(NULL, NULL);
	challengeTimeouts = g_hash_table_new(NULL, NULL);

	plugin.name = "keepalive";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	HOOK_ATTACH(irc_line, remote_line);
	HOOK_ATTACH(irc_disconnect, remote_disconnect);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_line, remote_line);
	HOOK_DETACH(socket_disconnect, remote_disconnect);

	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);

	// Since all plugins and their timeouts are already freed, we don't need to remove the contents
	g_hash_table_destroy(challenges);
	g_hash_table_destroy(challengeTimeouts);
}

HOOK_LISTENER(remote_line)
{
	IrcConnection *irc = HOOK_ARG(IrcConnection *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	IrcProxy *proxy;
	if((proxy = $(IrcProxy *, irc_proxy, getIrcProxyByIrcConnection)(irc)) != NULL) { // this IRC connection is actually proxied
		if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(proxy, "keepalive")) { // plugin is enabled for this proxy
			if(g_strcmp0(message->command, "PONG") == 0 && message->trailing != NULL) { // There is a pong from the server
				GTimeVal *timer;
				if((timer = g_hash_table_lookup(challengeTimeouts, proxy)) != NULL) {
					GString *challenge = g_string_new("");
					g_string_append_printf(challenge, "%ld%ld", timer->tv_sec, timer->tv_usec);
					if(g_strcmp0(message->trailing, challenge->str) == 0) { // Matches our challenge
						LOG_DEBUG("Successful keepalive response from IRC connection %d: %ld%ld", proxy->irc->socket->fd, timer->tv_sec, timer->tv_usec);
						// Stop the timer
						TIMER_DEL(timer);
					}
					g_string_free(challenge, true);
				}
			}
		}
	}
}

HOOK_LISTENER(remote_disconnect)
{
	IrcConnection *irc = HOOK_ARG(IrcConnection *);

	IrcProxy *proxy;
	if((proxy = $(IrcProxy *, irc_proxy, getIrcProxyByIrcConnection)(irc)) != NULL) { // a proxy socket disconnected
		if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(proxy, "keepalive")) { // plugin is enabled for this proxy
			LOG_INFO("Remote IRC connection for IRC proxy on port %s disconnected", proxy->server->port);
			reconnectRemoteConnection(proxy);
		}
	}
}

TIMER_CALLBACK(challenge)
{
	IrcProxy *proxy = custom_data;
	GTimeVal *timeout;

	if(proxy->irc->socket->connected) { // Only schedule a challenge if the socket is connected
		// Schedule an expiration time for the challenge
		if((timeout = TIMER_ADD_TIMEOUT_EX(keepaliveTimeout, challenge_timeout, proxy)) == NULL) {
			LOG_ERROR("Failed to add IRC proxy keepalive timeout for IRC proxy on port %s", proxy->server->port);
			return;
		}

		// Add challenge to hash table
		g_hash_table_insert(challengeTimeouts, proxy, timeout);

		// Send challenge to remote IRC server
		$(bool, irc, ircSend)(proxy->irc, "PING :%ld%ld", timeout->tv_sec, timeout->tv_usec);
		LOG_DEBUG("Sending keepalive challenge to IRC connection %d: %ld%ld", proxy->irc->socket->fd, timeout->tv_sec, timeout->tv_usec);
	} else {
		// Socket is no longer connected, try to reconnect it
		reconnectRemoteConnection(proxy);
	}

	// Schedule a new challenge
	if((timeout = TIMER_ADD_TIMEOUT_EX(keepaliveInterval, challenge, proxy)) == NULL) {
		LOG_ERROR("Failed to add IRC proxy keepalive timeout for IRC proxy on port %s", proxy->server->port);
		return;
	}

	// Replace the old challenge with the new one
	g_hash_table_replace(challenges, proxy, timeout);
}

TIMER_CALLBACK(challenge_timeout)
{
	IrcProxy *proxy = custom_data;

	LOG_ERROR("Keepalive challenge timed out for remote IRC connection %d of IRC proxy on port %s", proxy->irc->socket->fd, proxy->server->port);

	// Disconnect socket so the disconnect hook will reconnect it
	$(bool, socket, disconnectSocket)(proxy->irc->socket);

	// Remove the challenge from the challenge table
	g_hash_table_remove(challengeTimeouts, &time);
}

/**
 * Reconnects a remote IRC connection for an IRC proxy
 *
 * @param proxy		the IRC proxy of which the remote IRC connection should be reloaded
 */
static void reconnectRemoteConnection(IrcProxy *proxy)
{
	LOG_DEBUG("Trying to reconnect remote IRC connection for IRC proxy on port %s", proxy->server->port);
	// Now try to reconnect
	if($(bool, socket, connectSocket)(proxy->irc->socket) && $(bool, socket, enableSocketPolling)(proxy->irc->socket)) { // reconnect successful
		LOG_INFO("Remote IRC connection for IRC proxy on port %s reconnected successfully", proxy->server->port);

		// Reauthenticate the connection
		$(void, irc, authenticateIrcConnection)(proxy->irc);

		// Reschedule the keepalive timer for this connection
		GTimeVal *timeout;
		if((timeout = g_hash_table_lookup(challenges, proxy)) != NULL) { // check if there is a challenge going on right now that hasn't timed out yet
			if(TIMER_DEL(timeout)) { // clear the challenge timeout timer as well
				// Remove the challenge from the challenge table
				g_hash_table_remove(challenges, proxy);

				// Schedule a new challenge timre
				if((timeout = TIMER_ADD_TIMEOUT_EX(keepaliveInterval, challenge, proxy)) == NULL) {
					LOG_ERROR("Failed to add IRC proxy keepalive challenge for IRC proxy on port %s", proxy->server->port);
					return;
				}

				g_hash_table_insert(challenges, proxy, timeout);
			}
		}
	}
}

/**
 * Initializes the plugin
 *
 * @param proxy		the IRC proxy to initialize the plugin for
 * @param name		the name of the IRC proxy plugin to finalize
 * @result			true if successful
 */
static bool initPlugin(IrcProxy *proxy, char *name)
{
	GTimeVal *timeout;

	if((timeout = TIMER_ADD_TIMEOUT_EX(keepaliveInterval, challenge, proxy)) == NULL) {
		LOG_ERROR("Failed to add IRC proxy keepalive timeout for IRC proxy on port %s", proxy->server->port);
		return false;
	}

	g_hash_table_insert(challenges, proxy, timeout);

	return true;
}

/**
 * Finalizes the plugin
 *
 * @param proxy		the IRC proxy to finalize the plugin for
 * @param name		the name of the IRC proxy plugin to finalize
 */
static void finiPlugin(IrcProxy *proxy, char *name)
{
	// Fetch the timeout for this proxy from the timeouts table
	GTimeVal *timeout = g_hash_table_lookup(challenges, proxy);

	// Free the timeout from the timeouts table
	g_hash_table_remove(challenges, proxy);

	// Remove the timeout
	TIMER_DEL(timeout);

	if((timeout = g_hash_table_lookup(challengeTimeouts, proxy)) != NULL) { // check if there is a challenge going on right now that hasn't timed out yet
		// Remove the challenge timeout from the challenge table
		g_hash_table_remove(challengeTimeouts, proxy);

		TIMER_DEL(timeout); // clear the challenge timeout timer as well
	}
}
