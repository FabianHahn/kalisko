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
#include "modules/irc/irc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_channel/irc_channel.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/event/event.h"
#define API

MODULE_NAME("irc_bouncer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a multi-user multi-connection IRC bouncer service that can be configured via the standard config");
MODULE_VERSION(0, 3, 9);
MODULE_BCVERSION(0, 3, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_channel", 0, 1, 4), MODULE_DEPENDENCY("irc", 0, 5, 0), MODULE_DEPENDENCY("irc_proxy", 0, 3, 6), MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_bouncerReattached(void *subject, const char *event, void *data, va_list args);
static IrcProxy *createIrcProxyByStore(char *name, Store *config);

/**
 * A hash table that associates names with their corresponding IrcProxy objects
 */
static GHashTable *proxies;

MODULE_INIT
{
	proxies = g_hash_table_new(&g_str_hash, &g_str_equal);

	Store *bouncers;

	if((bouncers = getConfigPath("irc/bouncers")) == NULL || bouncers->type != STORE_ARRAY) {
		logError("Could not find required config value 'irc/bouncers' for this profile, aborting IRC bouncer");
		return false;
	}

	GHashTableIter iter;
	char *name;
	Store *bnc;

	g_hash_table_iter_init(&iter, bouncers->content.array);
	while(g_hash_table_iter_next(&iter, (void *) &name, (void *) &bnc)) {
		IrcProxy *proxy;
		if((proxy = createIrcProxyByStore(strdup(name), bnc)) == NULL) { // creating bouncer failed
			logWarning("Failed to create IRC proxy for IRC bouncer configuration '%s', skipping", name);
		} else {
			attachEventListener(proxy, "client_authenticated", NULL, &listener_bouncerReattached);
			logNotice("Successfully created an IRC proxy for IRC bouncer configuration '%s'", name);
			g_hash_table_insert(proxies, proxy->name, proxy); // add the proxy to the table
		}
	}

	return true;
}

MODULE_FINALIZE
{
	GHashTableIter iter;
	char *name;
	IrcProxy *proxy;

	g_hash_table_iter_init(&iter, proxies);
	while(g_hash_table_iter_next(&iter, (void *) &name, (void *) &proxy)) {
		detachEventListener(proxy, "client_authenticated", NULL, &listener_bouncerReattached);
		disableIrcProxyPlugins(proxy); // disable plugins of the proxy
		IrcConnection *irc = proxy->irc; // backup the IRC connection
		freeIrcProxy(proxy); // free the proxy
		freeIrcConnection(irc); // now free the associated IRC connection
	}

	g_hash_table_destroy(proxies);
}

static void listener_bouncerReattached(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);
	IrcConnection *irc = client->proxy->irc;

	if(g_hash_table_lookup(proxies, client->proxy->name) != NULL) { // this proxy is actually a bouncer proxy
		GList *channels = getTrackedChannels(irc);

		for(GList *iter = channels; iter != NULL; iter = iter->next) {
			IrcChannel *channel = iter->data;
			proxyClientIrcSend(client, ":%s!%s@%s JOIN %s", irc->nick, irc->user, irc->socket->host, channel->name);
			ircSend(irc, "NAMES %s", channel->name);
			ircSend(irc, "TOPIC %s", channel->name);
		}

		triggerEvent(client->proxy, "bouncer_reattached", client);

		g_list_free(channels);
	}
}

/**
 * Creates an IRC proxy for the IRC bouncer by passing a config store specifying the remote IRC connection to use as well as connection and user data fr the server
 *
 * @param name			the name of the proxy to create
 * @param config		config store containing params for the proxy
 * @result				true if successful
 */
static IrcProxy *createIrcProxyByStore(char *name, Store *config)
{
	IrcConnection *irc;
	IrcProxy *proxy;

	Store *remote;

	if((remote = getStorePath(config, "remote")) == NULL) {
		logError("Could not find required config value 'remote' for IRC bouncer configuration '%s', aborting IRC proxy", name);
		return NULL;
	}

	if((irc = createIrcConnectionByStore(remote)) == NULL) {
		logError("Failed to establich remote IRC connection, aborting IRC proxy");
		return NULL;
	}

	if(!enableChannelTracking(irc)) {
		logError("Failed to enable channel tracking for remote IRC connection %d, aborting IRC proxy", irc->socket->fd);
		freeIrcConnection(irc);
		return NULL;
	}

	logNotice("Successfully established remote IRC connection for IRC proxy");

	Store *param;
	char *password;

	if((param = getStorePath(config, "password")) == NULL || param->type != STORE_STRING) {
		logError("Could not find required config value 'password' for IRC bouncer configuration '%s', aborting IRC proxy", name);
		freeIrcConnection(irc);
		return NULL;
	}

	password = strdup(param->content.string);

	if((proxy = createIrcProxy(name, irc, password)) == NULL) {
		logError("Failed to create IRC proxy for IRC  configuration '%s', aborting", name);
		freeIrcConnection(irc);
		return false;
	}

	// Enable plugin support
	if(!enableIrcProxyPlugins(proxy)) {
		freeIrcProxy(proxy);
		freeIrcConnection(irc);
		logError("Failed to enable IRC proxy plugins for IRC configuration '%s', aborting", name);
		return false;
	}

	// Enable plugins listed in config / params
	if((param = getStorePath(config, "plugins")) != NULL && param->type == STORE_LIST) {
		GQueue *plugins = param->content.list;
		unsigned int i = 0;

		for(GList *iter = plugins->head; iter != NULL; iter = iter->next, i++) {
			Store *pentry = iter->data;

			if(pentry->type != STORE_STRING) {
				freeIrcProxy(proxy);
				freeIrcConnection(irc);
				logError("Element %d of param list 'plugins' for IRC bouncer configuration '%s' is not a string but of store type %d, aborting", i, name, pentry->type);
				return false;
			}

			char *pname = pentry->content.string;

			if(!enableIrcProxyPlugin(proxy, pname)) {
				logWarning("Failed to enable perform plugin %s for IRC bouncer configuration '%s', skipping", pname, name);
			}
		}
	}

	return proxy;
}
