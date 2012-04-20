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


#include <glib.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/event/event.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#define API

MODULE_NAME("ircpp_perform");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that performs a predefined set of actions after reconnecting to a remote IRC server");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 0), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 1), MODULE_DEPENDENCY("event", 0, 1, 2), MODULE_DEPENDENCY("config", 0, 4, 2), MODULE_DEPENDENCY("store", 0, 6, 11));

static void listener_remoteReconnect(void *subject, const char *event, void *data, va_list args);
static void listener_clientLine(void *subject, const char *event, void *data, va_list args);
static void listener_clientAuthenticated(void *subject, const char *event, void *data, va_list args);
static void listener_clientDisconnected(void *subject, const char *event, void *data, va_list args);
static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);
static Store *getPluginConfig();
static Store *getProxyConfig(IrcProxy *proxy);
static void executePerformList(IrcProxy *proxy);


static IrcProxyPlugin plugin;

MODULE_INIT
{
	plugin.name = "perform";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!addIrcProxyPlugin(&plugin)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	delIrcProxyPlugin(&plugin);
}

static void listener_remoteReconnect(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;

	IrcProxy *proxy;
	if((proxy = getIrcProxyByIrcConnection(irc)) != NULL) { // a proxy socket reconnected
		if(isIrcProxyPluginEnabled(proxy, "perform")) { // plugin is enabled for this proxy
			executePerformList(proxy);
		}
	}
}

static void listener_clientLine(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if(isIrcProxyPluginEnabled(client->proxy, "perform")) { // plugin is enabled for this proxy
		if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 && g_strcmp0(message->params[0], "*perform") == 0 && message->trailing != NULL) { // there is a message to our virtual bot
			char **parts = g_strsplit(message->trailing, " ", 0);
			int count;

			// Find number of parts
			for(count = 0; parts[count] != NULL; count++);

			if(count > 0) {
				if(g_strcmp0(parts[0], "help") == 0) {
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :The following commands are available for the %cperform%c IRC proxy plugin:", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%chelp%c               displays this help message", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%clist%c               shows the currently set perform list", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%cclear%c              clears currently set perform list", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%cdelete%c [number]    removed the entry with the chosen number from the perform list", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%cadd%c [command]      adds a command to the perform list", client->proxy->irc->nick, (char) 2, (char) 2);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%cexecute%c            executes the currently set perform list", client->proxy->irc->nick, (char) 2, (char) 2);
				} else if(g_strcmp0(parts[0], "list") == 0) {
					Store *proxyConfig = getProxyConfig(client->proxy);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Perform list for IRC proxy %c%s%c:", client->proxy->irc->nick, (char) 2, client->proxy->name, (char) 2);
					int i = 0;
					for(GList *iter = proxyConfig->content.list->head; iter != NULL; iter = iter->next, i++) {
						Store *value = iter->data;
						if(value->type == STORE_STRING) {
							proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :%c#%d%c: %s", client->proxy->irc->nick, (char) 2, i, (char) 2, value->content.string);
						}
					}

					if(i == 0) {
						proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Perform list currently empty!", client->proxy->irc->nick);
					}
				} else if(g_strcmp0(parts[0], "clear") == 0) {
					Store *proxyConfig = getProxyConfig(client->proxy);
					int i = 0;
					for(GList *iter = proxyConfig->content.list->head; iter != NULL; iter = iter->next, i++) {
						Store *value = iter->data;
						freeStore(value);
					}
					g_queue_clear(proxyConfig->content.list);
					saveWritableConfig();

					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Cleared %c%d%c items from perform list for IRC proxy %c%s%c.", client->proxy->irc->nick, (char) 2, i, (char) 2, (char) 2, client->proxy->name, (char) 2);
				} else if(g_strcmp0(parts[0], "delete") == 0) {
					if(count > 1) {
						Store *proxyConfig = getProxyConfig(client->proxy);
						int n = atoi(parts[1]);

						Store *value = g_queue_pop_nth(proxyConfig->content.list, n);
						if(value != NULL) {
							if(value->type == STORE_STRING) {
								proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Cleared item %c#%d%c from perform list for IRC proxy %c%s%c: %s", client->proxy->irc->nick, (char) 2, n, (char) 2, (char) 2, client->proxy->name, (char) 2, value->content.string);
							}
							freeStore(value);
						}

						saveWritableConfig();
					}
				} else if(g_strcmp0(parts[0], "add") == 0) {
					if(count > 1) {
						Store *proxyConfig = getProxyConfig(client->proxy);
						char *command = g_strjoinv(" ", &parts[1]); // join up arguments starting from first

						g_queue_push_tail(proxyConfig->content.list, createStoreStringValue(command));
						saveWritableConfig();

						proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Added item to perform list for IRC proxy %c%s%c: %s", client->proxy->irc->nick, (char) 2, client->proxy->name, (char) 2, command);
					}
				} else if(g_strcmp0(parts[0], "execute") == 0) {
					executePerformList(client->proxy);
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Executed perform list for IRC proxy %c%s%c.", client->proxy->irc->nick, (char) 2, client->proxy->name, (char) 2);
				} else {
					proxyClientIrcSend(client, ":*perform!kalisko@kalisko.proxy PRIVMSG %s :Command not understood. Use the %chelp%c command to get alist of all available commands.", client->proxy->irc->nick, (char) 2, (char) 2);
				}
			}

			g_strfreev(parts);
		}
	}
}

static void listener_clientAuthenticated(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	attachEventListener(client, "line", NULL, &listener_clientLine);
}

static void listener_clientDisconnected(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	detachEventListener(client, "line", NULL, &listener_clientLine);
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
	// Attach to existing clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		attachEventListener(client, "line", NULL, &listener_clientLine);
	}

	addIrcProxyRelayException(proxy, "*perform");
	attachEventListener(proxy->irc, "reconnect", NULL, &listener_remoteReconnect);
	attachEventListener(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	attachEventListener(proxy, "client_disconnected", NULL, &listener_clientDisconnected);

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
	delIrcProxyRelayException(proxy, "*perform");
	detachEventListener(proxy->irc, "reconnect", NULL, &listener_remoteReconnect);
	detachEventListener(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	detachEventListener(proxy, "client_disconnected", NULL, &listener_clientDisconnected);

	// Detach from remaining clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		detachEventListener(client, "line", NULL, &listener_clientLine);
	}
}

/**
 * Retrieves the writable config store for this plugin
 *
 * @result			the writable config store or NULL on failure
 */
static Store *getPluginConfig()
{
	Store *config = getWritableConfig();

	if(getStorePath(config, "irc") == NULL) {
		setStorePath(config, "irc", createStore());
	}

	Store *configIrcPerform;
	if((configIrcPerform = getStorePath(config, "irc/perform")) == NULL) {
		configIrcPerform = createStore();
		setStorePath(config, "irc/perform", configIrcPerform);
	}

	return configIrcPerform;
}

/**
 * Retrieves the writable config store for one of the IRC proxies using this plugin
 *
 * @param proxy		the proxy for which to retrieve a writable config
 * @result			the writable config store or NULL on failure
 */
static Store *getProxyConfig(IrcProxy *proxy)
{
	Store *config = getPluginConfig();

	Store *configProxy;
	if((configProxy = getStorePath(config, proxy->name)) == NULL || configProxy->type != STORE_LIST) {
		deleteStorePath(config, proxy->name);
		configProxy = createStoreListValue(NULL);
		setStorePath(config, proxy->name, configProxy);
	}

	return configProxy;
}

/**
 * Executes the currently set perform list for an IRC proxy
 *
 * @param proxy			the IRC proxy for which to execute the perform list
 */
static void executePerformList(IrcProxy *proxy)
{
	Store *configProxy = getProxyConfig(proxy);

	int i = 0;
	for(GList *iter = configProxy->content.list->head; iter != NULL; iter = iter->next, i++) {
		Store *value = iter->data;
		if(value->type == STORE_STRING) {
			ircSend(proxy->irc, "%s", value->content.string);
		}
	}

	LOG_INFO("Executed %d perform commands for IRC proxy '%s'", i, proxy->name);
}
