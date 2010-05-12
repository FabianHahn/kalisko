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
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "api.h"

MODULE_NAME("ircpp_log");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that allows log messages to be relayed to IRC proxy clients");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 6), MODULE_DEPENDENCY("irc_parser", 0, 1, 1));

HOOK_LISTENER(log);
static bool initPlugin(IrcProxy *proxy);
static void finiPlugin(IrcProxy *proxy);
static IrcProxyPlugin plugin;

/**
 * A queue with proxies that have the plugin enabled
 */
static GQueue *proxies;

MODULE_INIT
{
	proxies = g_queue_new();

	plugin.name = "log";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	HOOK_ATTACH(log, log);

	return true;
}

MODULE_FINALIZE
{
	g_queue_free(proxies);

	HOOK_DETACH(log, log);

	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);
}

HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GString *msg = g_string_new("");

	switch(type) {
		case LOG_TYPE_DEBUG:
			g_string_append_printf(msg, "(%c3debug%c) %s", (char) 3, (char) 0x0f, message);
		break;
		case LOG_TYPE_INFO:
			g_string_append_printf(msg, "(%c12info%c) %s", (char) 3, (char) 0x0f, message);
		break;
		case LOG_TYPE_WARNING:
			g_string_append_printf(msg, "(%c7warning%c) %s", (char) 3, (char) 0x0f, message);
		break;
		case LOG_TYPE_ERROR:
			g_string_append_printf(msg, "(%c4error%c) %s", (char) 3, (char) 0x0f, message);
		break;
	}

	for(GList *iter = proxies->head; iter != NULL; iter = iter->next) {
		IrcProxy *proxy = iter->data;

		for(GList *citer = proxy->clients->head; citer != NULL; citer = citer->next) {
			IrcProxyClient *client = citer->data;

			$(bool, irc_proxy, proxyClientIrcSend)(client, ":*log!kalisko@kalisko.proxy PRIVMSG %s :%s", client->proxy->irc->nick, msg->str);
		}
	}

	g_string_free(msg, true);
}

/**
 * Initializes the plugin
 *
 * @param proxy		the IRC proxy to initialize the plugin for
 * @result			true if successful
 */
static bool initPlugin(IrcProxy *proxy)
{
	g_queue_push_head(proxies, proxy);

	return true;
}

/**
 * Finalizes the plugin
 *
 * @param proxy		the IRC proxy to finalize the plugin for
 */
static void finiPlugin(IrcProxy *proxy)
{
	g_queue_remove(proxies, proxy);
}
