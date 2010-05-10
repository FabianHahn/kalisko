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
#include "modules/irc/irc.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"

MODULE_NAME("ircpp_keepalive");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that tries to keep the connection to the remote IRC server alive by pinging it in regular intervals");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 3, 3), MODULE_DEPENDENCY("irc_proxy", 0, 1, 13), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 5), MODULE_DEPENDENCY("irc_parser", 0, 1, 1));

#ifndef IRCPP_KEEPALIVE_INTERVAL
#define IRCPP_KEEPALIVE_INTERVAL (10 * G_USEC_PER_SEC)
#endif

#ifndef IRCPP_KEEPALIVE_TIMEOUT
#define IRCPP_KEEPALIVE_TIMEOUT (5 * G_USEC_PER_SEC)
#endif

TIMER_CALLBACK(IRCPP_KEEPALIVE_CHALLENGE);
TIMER_CALLBACK(IRCPP_KEEPALIVE_CHALLENGE_TIMEOUT);
HOOK_LISTENER(remote_line);
static bool initPlugin(IrcProxy *proxy);
static void finiPlugin(IrcProxy *proxy);
static IrcProxyPlugin plugin;

/**
 * Hash table to associate IrcProxy objects with the GTimeVal objects for the keepalive challenges
 */
static GHashTable *challenges;

/**
 * Hash table to associate IrcProxy objects with the GTimeVal objects for the keepalive challenge timeouts
 */
static GHashTable *challengeTimeouts;

MODULE_INIT
{
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

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_line, remote_line);

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


TIMER_CALLBACK(IRCPP_KEEPALIVE_CHALLENGE)
{
	IrcProxy *proxy = custom_data;
	GTimeVal *timeout;

	// Schedule an expiration time for the challenge
	timeout = TIMER_ADD_TIMEOUT_EX(IRCPP_KEEPALIVE_TIMEOUT, IRCPP_KEEPALIVE_CHALLENGE_TIMEOUT, proxy);
	// Add challenge to hash table
	g_hash_table_insert(challengeTimeouts, proxy, timeout);

	// Send challenge to remote IRC server
	$(bool, irc, ircSend)(proxy->irc, "PING :%ld%ld", timeout->tv_sec, timeout->tv_usec);
	LOG_DEBUG("Sending keepalive challenge to IRC connection %d: %ld%ld", proxy->irc->socket->fd, timeout->tv_sec, timeout->tv_usec);

	// Schedule a new challenge
	timeout = TIMER_ADD_TIMEOUT_EX(IRCPP_KEEPALIVE_INTERVAL, IRCPP_KEEPALIVE_CHALLENGE, proxy);
	// Replace the old challenge with the new one
	g_hash_table_replace(challenges, proxy, timeout);
}

TIMER_CALLBACK(IRCPP_KEEPALIVE_CHALLENGE_TIMEOUT)
{
	IrcProxy *proxy = custom_data;

	LOG_ERROR("Keepalive challenge timed out for IRC connection %d", proxy->irc->socket->fd);

	// TODO: reconnect etc

	// Remove the challenge from the challenge table
	g_hash_table_remove(challengeTimeouts, &time);
}

/**
 * Initializes the plugin
 *
 * @param proxy		the IRC proxy to initialize the plugin for
 * @result			true if successful
 */
static bool initPlugin(IrcProxy *proxy)
{
	GTimeVal *timeout = TIMER_ADD_TIMEOUT_EX(IRCPP_KEEPALIVE_INTERVAL, IRCPP_KEEPALIVE_CHALLENGE, proxy);
	g_hash_table_insert(challenges, proxy, timeout);

	return true;
}

/**
 * Finalizes the plugin
 *
 * @param proxy		the IRC proxy to finalize the plugin for
 */
static void finiPlugin(IrcProxy *proxy)
{
	// Fetch the timeout for this proxy from the timeouts table
	GTimeVal *timeout = g_hash_table_lookup(challenges, proxy);

	// Remove the timeout
	TIMER_DEL(timeout);

	// Free the timeout from the timeouts table
	g_hash_table_remove(challenges, proxy);

	if((timeout = g_hash_table_lookup(challengeTimeouts, proxy)) != NULL) { // check if there is a challenge going on right now that hasn't timed out yet
		TIMER_DEL(timeout); // clear the challenge timeout timer as well

		// Remove it too
		g_hash_table_remove(challengeTimeouts, proxy);
	}
}
