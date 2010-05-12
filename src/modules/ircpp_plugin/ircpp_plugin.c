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

MODULE_NAME("ircpp_plugin");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that allows proxy clients to load or unload other IRC proxy plugins");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 6), MODULE_DEPENDENCY("irc_parser", 0, 1, 1));

HOOK_LISTENER(client_line);
static bool initPlugin(IrcProxy *proxy);
static void finiPlugin(IrcProxy *proxy);
static IrcProxyPlugin plugin;

MODULE_INIT
{
	plugin.name = "plugin";
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
}

HOOK_LISTENER(client_line)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, "plugin")) { // plugin is enabled for this proxy
		if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 && g_strcmp0(message->params[0], "*plugin") == 0 && message->trailing != NULL) { // there is a message to our virtual bot
			char **parts = g_strsplit(message->trailing, " ", 0);
			int count;

			// Find number of parts
			for(count = 0; parts[count] != NULL; count++);

			if(count > 0) {
				if(g_strcmp0(parts[0], "help") == 0) {
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :The following commands are available for the %cplugin%c IRC proxy plugin:", client->proxy->irc->nick, (char) 2, (char) 2);
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%chelp%c             displays this help message", client->proxy->irc->nick, (char) 2, (char) 2);
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%clist%c             lists all available modules", client->proxy->irc->nick, (char) 2, (char) 2);
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%cload%c [plugin]    loads a plugin", client->proxy->irc->nick, (char) 2, (char) 2);
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%cunload%c [plugin]  unloads a plugin", client->proxy->irc->nick, (char) 2, (char) 2);
				} else if(g_strcmp0(parts[0], "list") == 0) {
					GList *plugins = $(GList *, irc_proxy_plugin, getAvailableIrcProxyPlugins)();

					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :The following IRC proxy plugins are available (bold ones are loaded):", client->proxy->irc->nick);

					// Iterate over all plugins and check if they're loaded
					for(GList *iter = plugins; iter != NULL; iter = iter->next) {
						char *pname = iter->data;

						if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, pname)) {
							$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%c%s%c", client->proxy->irc->nick, (char) 2, pname, (char) 2);
						} else {
							$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :%s", client->proxy->irc->nick, pname);
						}
					}

					g_list_free(plugins);
				} else if(g_strcmp0(parts[0], "load") == 0 && count > 1) {
					if($(bool, irc_proxy_plugin, enableIrcProxyPlugin)(client->proxy, parts[1])) {
						$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :Successfully loaded IRC proxy plugin %c%s%c", client->proxy->irc->nick, (char) 2, parts[1], (char) 2);
					} else {
						$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :Failed to load IRC proxy plugin %c%s%c, please check the error log", client->proxy->irc->nick, (char) 2, parts[1], (char) 2);
					}
				} else if(g_strcmp0(parts[0], "unload") == 0 && count > 1) {
					if($(bool, irc_proxy_plugin, disableIrcProxyPlugin)(client->proxy, parts[1])) {
						$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :Successfully unloaded IRC proxy plugin %c%s%c", client->proxy->irc->nick, (char) 2, parts[1], (char) 2);
					} else {
						$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :Failed to unload IRC proxy plugin %c%s%c, please check the error log", client->proxy->irc->nick, (char) 2, parts[1], (char) 2);
					}
				} else {
					$(bool, irc_proxy, proxyClientIrcSend)(client, ":*plugin!kalisko@kalisko.proxy PRIVMSG %s :Command not understood. Use the %chelp%c command to get alist of all available commands", client->proxy->irc->nick, (char) 2, (char) 2);
				}
			}

			g_strfreev(parts);
		}
	}
}

/**
 * Initializes the plugin
 *
 * @param proxy		the IRC proxy to initialize the plugin for
 * @result			true if successful
 */
static bool initPlugin(IrcProxy *proxy)
{
	$(void, irc_proxy, addIrcProxyRelayException)(proxy, "*plugin");

	return true;
}

/**
 * Finalizes the plugin
 *
 * @param proxy		the IRC proxy to finalize the plugin for
 */
static void finiPlugin(IrcProxy *proxy)
{
	$(void, irc_proxy, delIrcProxyRelayException)(proxy, "*plugin");
}
