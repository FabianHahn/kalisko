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
#include "modules/irc/irc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_channel/irc_channel.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "api.h"

MODULE_NAME("irc_bouncer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a multi-user multi-connection IRC bouncer service that can be configured via the standard config");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_channel", 0, 1, 4), MODULE_DEPENDENCY("irc", 0, 2, 7), MODULE_DEPENDENCY("irc_proxy", 0, 2, 0), MODULE_DEPENDENCY("config", 0, 3, 0), MODULE_DEPENDENCY("store", 0, 5, 3));

HOOK_LISTENER(bouncer_reattach);
static IrcProxy *createIrcProxyByStore(int id, Store *config);

/**
 * An ordered list conaining either an IrcProxy object or NULL for every bouncer configuration
 */
static GQueue *proxies;

MODULE_INIT
{
	proxies = g_queue_new();

	Store *bouncers;

	if((bouncers = $(Store *, config, getConfigPath)("irc/proxy/bouncers")) == NULL || bouncers->type != STORE_LIST) {
		LOG_ERROR("Could not find required config value 'irc/proxy/bouncers' for this profile, aborting IRC bouncer");
		return false;
	}

	GQueue *bncs = bouncers->content.list;
	int i = 0;
	for(GList *iter = bncs->head; iter != NULL; iter = iter->next, i++) { // iterate over all bouncer configurations
		Store *bnc = iter->data;

		IrcProxy *proxy;
		if((proxy = createIrcProxyByStore(i, bnc)) == NULL) { // creating bouncer failed
			LOG_WARNING("Failed to create IRC proxy for bouncer configuration %d, skipping", i);
		}

		g_queue_push_tail(proxies, proxy); // push the proxy to the end of the queue
	}

	HOOK_ATTACH(irc_proxy_client_authenticated, bouncer_reattach);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_proxy_client_authenticated, bouncer_reattach);

	for(GList *iter = proxies->head; iter != NULL; iter = iter->next) {
		IrcProxy *proxy = iter->data;

		if(proxy != NULL) { // If the proxy was actually created before
			IrcConnection *irc = proxy->irc;
			$(void, irc_proxy_plugins, disableIrcProxyPlugins)(proxy); // disable plugins of the proxy
			$(void, irc_proxy, freeIrcProxy)(proxy); // disable the proxy itself
			$(void, irc, freeIrcConnection)(irc); // free the remote connection
		}
	}

	g_queue_free(proxies);
}

HOOK_LISTENER(bouncer_reattach)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);
	IrcConnection *irc = client->proxy->irc;

	GList *channels = $(GList *, irc_channel, getTrackedChannels)(irc);

	for(GList *iter = channels; iter != NULL; iter = iter->next) {
		IrcChannel *channel = iter->data;
		$(bool, irc_proxy, proxyClientIrcSend)(client, ":%s!%s@%s JOIN %s", irc->nick, irc->user, irc->socket->host, channel->name);
		$(bool, irc, ircSend)(irc, "NAMES %s", channel->name);
		$(bool, irc, ircSend)(irc, "TOPIC %s", channel->name);
	}

	g_list_free(channels);
}

/**
 * Creates an IRC proxy for the IRC bouncer by passing a config store specifying the remote IRC connection to use as well as connection and user data fr the server
 *
 * @param id			the ID of the proxy to create
 * @param config		config store containing params for the proxy
 * @result				true if successful
 */
static IrcProxy *createIrcProxyByStore(int id, Store *config)
{
	IrcConnection *irc;
	IrcProxy *proxy;

	Store *remote;

	if((remote = $(Store *, store, getStorePath)(config, "remote")) == NULL) {
		LOG_ERROR("Could not find required config value 'remote' for IRC bouncer configuration %d, aborting IRC proxy", id);
		return NULL;
	}

	if((irc = $(IrcConnection *, irc, createIrcConnectionByStore)(remote)) == NULL) {
		LOG_ERROR("Failed to establich remote IRC connection, aborting IRC proxy");
		return NULL;
	}

	if(!$(bool, irc_channel, enableChannelTracking)(irc)) {
		LOG_ERROR("Failed to enable channel tracking for remote IRC connection %d, aborting IRC proxy", irc->socket->fd);
		$(void, irc, freeIrcConnection)(irc);
		return NULL;
	}

	LOG_INFO("Successfully established remote IRC connection for IRC proxy");

	Store *param;
	char *password;

	if((param = $(Store *, config, getStorePath)(config, "password")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required config value 'password' for IRC bouncer configuration %d, aborting IRC proxy", id);
		$(void, irc, freeIrcConnection)(irc);
		return NULL;
	}

	password = param->content.string;

	if((proxy = $(IrcProxy *, irc_proxy, createIrcProxy)(id, irc, password)) == NULL) {
		LOG_ERROR("Failed to create IRC proxy for IRC  configuration %d, aborting", id);
		$(void, irc, freeIrcConnection)(irc);
		return false;
	}

	// Enable plugin support
	if(!$(bool, irc_proxy_plugin, enableIrcProxyPlugins)(proxy)) {
		$(void, irc_proxy, freeIrcProxy)(proxy);
		$(void, irc, freeIrcConnection)(irc);
		LOG_ERROR("Failed to enable IRC proxy plugins for IRC configuration %d, aborting", id);
		return false;
	}

	// Enable plugins listed in config / params
	if((param = $(Store *, config, getStorePath)(config, "plugins")) != NULL && param->type == STORE_LIST) {
		GQueue *plugins = param->content.list;
		unsigned int i = 0;

		for(GList *iter = plugins->head; iter != NULL; iter = iter->next, i++) {
			Store *pentry = iter->data;

			if(pentry->type != STORE_STRING) {
				$(void, irc_proxy, freeIrcProxy)(proxy);
				$(void, irc, freeIrcConnection)(irc);
				LOG_ERROR("Element %d of param list 'plugins' for IRC bouncer configuration %d is not a string but of store type %d, aborting", i, id, pentry->type);
				return false;
			}

			char *pname = pentry->content.string;

			if(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, pname)) {
				LOG_WARNING("Failed to enable perform plugin %s for IRC bouncer configuration %d, skipping", pname, id);
			}
		}
	}

	return proxy;
}
