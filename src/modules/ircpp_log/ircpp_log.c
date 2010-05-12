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
MODULE_VERSION(0, 2, 1);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 1));

HOOK_LISTENER(log);
static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);
static IrcProxyPlugin plugin_debug;
static IrcProxyPlugin plugin_info;
static IrcProxyPlugin plugin_warning;
static IrcProxyPlugin plugin_error;

/**
 * A queue with proxies that have the plugin_debug enabled
 */
static GQueue *proxies_debug;

/**
 * A queue with proxies that have the plugin_info enabled
 */
static GQueue *proxies_info;

/**
 * A queue with proxies that have the plugin_warning enabled
 */
static GQueue *proxies_warning;

/**
 * A queue with proxies that have the plugin_error enabled
 */
static GQueue *proxies_error;

MODULE_INIT
{
	proxies_debug = g_queue_new();
	proxies_info = g_queue_new();
	proxies_warning = g_queue_new();
	proxies_error = g_queue_new();

	plugin_debug.name = "log_debug";
	plugin_debug.handlers = g_queue_new();
	plugin_debug.initialize = &initPlugin;
	plugin_debug.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin_debug)) {
		return false;
	}

	plugin_info.name = "log_info";
	plugin_info.handlers = g_queue_new();
	plugin_info.initialize = &initPlugin;
	plugin_info.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin_info)) {
		return false;
	}

	plugin_warning.name = "log_warning";
	plugin_warning.handlers = g_queue_new();
	plugin_warning.initialize = &initPlugin;
	plugin_warning.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin_warning)) {
		return false;
	}

	plugin_error.name = "log_error";
	plugin_error.handlers = g_queue_new();
	plugin_error.initialize = &initPlugin;
	plugin_error.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin_error)) {
		return false;
	}

	HOOK_ATTACH(log, log);

	return true;
}

MODULE_FINALIZE
{
	g_queue_free(proxies_debug);
	g_queue_free(proxies_info);
	g_queue_free(proxies_warning);
	g_queue_free(proxies_error);

	HOOK_DETACH(log, log);

	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin_debug);
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin_info);
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin_warning);
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin_error);
}

HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GString *msg = g_string_new("");

	GQueue *proxies = NULL;

	switch(type) {
		case LOG_TYPE_DEBUG:
			g_string_append_printf(msg, "(%c3debug%c) %s", (char) 3, (char) 0x0f, message);
			proxies = proxies_debug;
		break;
		case LOG_TYPE_INFO:
			g_string_append_printf(msg, "(%c12info%c) %s", (char) 3, (char) 0x0f, message);
			proxies = proxies_info;
		break;
		case LOG_TYPE_WARNING:
			g_string_append_printf(msg, "(%c7warning%c) %s", (char) 3, (char) 0x0f, message);
			proxies = proxies_warning;
		break;
		case LOG_TYPE_ERROR:
			g_string_append_printf(msg, "(%c4error%c) %s", (char) 3, (char) 0x0f, message);
			proxies = proxies_error;
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
 * @param name		the name of the IRC proxy plugin to initialize
 * @result			true if successful
 */
static bool initPlugin(IrcProxy *proxy, char *name)
{
	if(g_strcmp0(name, "log_debug") == 0) {
		g_queue_push_head(proxies_debug, proxy);
	} else if(g_strcmp0(name, "log_info") == 0) {
		g_queue_push_head(proxies_info, proxy);
	} else if(g_strcmp0(name, "log_warning") == 0) {
		g_queue_push_head(proxies_warning, proxy);
	} else if(g_strcmp0(name, "log_error") == 0) {
		g_queue_push_head(proxies_error, proxy);
	} else {
		return false;
	}

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
	if(g_strcmp0(name, "log_debug") == 0) {
		g_queue_remove(proxies_debug, proxy);
	} else if(g_strcmp0(name, "log_info") == 0) {
		g_queue_remove(proxies_info, proxy);
	} else if(g_strcmp0(name, "log_warning") == 0) {
		g_queue_remove(proxies_warning, proxy);
	} else if(g_strcmp0(name, "log_error") == 0) {
		g_queue_remove(proxies_error, proxy);
	}
}
