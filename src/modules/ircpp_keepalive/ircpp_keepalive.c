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
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"

MODULE_NAME("ircpp_keepalive");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that tries to keep the connection to the remote IRC server alive by pinging it in regular intervals");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 5), MODULE_DEPENDENCY("irc_parser", 0, 1, 1));

#ifndef IRCPP_KEEPALIVE_TIMEOUT
#define IRCPP_KEEPALIVE_TIMEOUT 10
#endif

TIMER_CALLBACK(IRCPP_KEEPALIVE);
HOOK_LISTENER(client_line);
static bool initPlugin(IrcProxy *proxy);
static void finiPlugin(IrcProxy *proxy);
static IrcProxyPlugin plugin;

/**
 * Hash table to associate IrcProxy objects with the GTimeVal objects for the keepalive timeouts
 */
static GHashTable *timeouts;

MODULE_INIT
{
	timeouts = g_hash_table_new(NULL, NULL);

	plugin.name = "keepalive";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	HOOK_ATTACH(irc_proxy_client_line, client_line);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_proxy_client_line, client_line);

	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);

	// Since all plugins and their timeouts are already freed, we don't need to remove the contents
	g_hash_table_destroy(timeouts);
}

HOOK_LISTENER(client_line)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, "keepalive")) { // plugin is enabled for this proxy

	}
}


TIMER_CALLBACK(IRCPP_KEEPALIVE)
{

}

static bool initPlugin(IrcProxy *proxy)
{
	GTimeVal *timeout = TIMER_ADD_TIMEOUT_EX(IRCPP_KEEPALIVE_TIMEOUT, IRCPP_KEEPALIVE, proxy);
	g_hash_table_insert(timeouts, proxy, timeout);

	return true;
}

static void finiPlugin(IrcProxy *proxy)
{
	// Fetch the timeout for this proxy from the timeouts table
	GTimeVal *timeout = g_hash_table_lookup(timeouts, proxy);

	// Remove the timeout
	TIMER_DEL(timeout);

	// Free the timeout from the timeouts table
	g_hash_table_remove(timeouts, proxy);
}
