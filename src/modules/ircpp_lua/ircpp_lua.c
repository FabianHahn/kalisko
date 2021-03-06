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
#include "log.h"
#include "types.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/lua/module_lua.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/string_util/string_util.h"
#include "modules/event/event.h"
#define API

MODULE_NAME("ircpp_lua");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that allows proxy clients to evaluate Lua code by sending private messages to a virtual *lua bot");
MODULE_VERSION(0, 2, 8);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 7), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("lua", 0, 8, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 1), MODULE_DEPENDENCY("string_util", 0, 1, 2), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_clientLine(void *subject, const char *event, void *data, va_list args);
static void listener_clientAuthenticated(void *subject, const char *event, void *data, va_list args);
static void listener_clientDisconnected(void *subject, const char *event, void *data, va_list args);
static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);
static IrcProxyPlugin plugin;

MODULE_INIT
{
	plugin.name = "lua";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);
}

static void listener_clientLine(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, "lua")) { // plugin is enabled for this proxy
		if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 && g_strcmp0(message->params[0], "*lua") == 0 && message->trailing != NULL) { // there is a lua command to evaluate
			if(!$(bool, lua, evaluateLua)(message->trailing)) {
				GString *err = g_string_new("Lua error: ");
				char *ret = $(char *, lua, popLuaString)();
				g_string_append(err, ret);
				$(bool, irc_proxy, proxyClientIrcSend)(client, ":*lua!kalisko@kalisko.proxy PRIVMSG %s :%s", client->proxy->irc->nick, ret);
				free(ret);
				g_string_free(err, true);
			} else {
				char *ret = $(char *, lua, popLuaString)();

				if(ret != NULL) {
					$(void, string_util, stripDuplicateNewlines)(ret);

					char **lines = g_strsplit(ret, "\n", 0);

					for(int i = 0; lines[i] != NULL; i++) {
						$(bool, irc_proxy, proxyClientIrcSend)(client, ":*lua!kalisko@kalisko.proxy PRIVMSG %s :%s", client->proxy->irc->nick, lines[i]);
					}

					g_strfreev(lines);
					free(ret);
				}
			}
		}
	}
}

static void listener_clientAuthenticated(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	$(void, event, attachEventListener)(client, "line", NULL, &listener_clientLine);
}

static void listener_clientDisconnected(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	$(void, event, detachEventListener)(client, "line", NULL, &listener_clientLine);
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
	// Attach to existing clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		$(void, event, attachEventListener)(client, "line", NULL, &listener_clientLine);
	}

	$(void, irc_proxy, addIrcProxyRelayException)(proxy, "*lua");
	$(void, event, attachEventListener)(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	$(void, event, attachEventListener)(proxy, "client_disconnected", NULL, &listener_clientDisconnected);

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
	$(void, irc_proxy, delIrcProxyRelayException)(proxy, "*lua");
	$(void, event, detachEventListener)(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	$(void, event, detachEventListener)(proxy, "client_disconnected", NULL, &listener_clientDisconnected);

	// Detach from remaining clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		$(void, event, detachEventListener)(client, "line", NULL, &listener_clientLine);
	}
}
