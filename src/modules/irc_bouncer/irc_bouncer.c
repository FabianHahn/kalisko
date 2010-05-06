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
MODULE_DESCRIPTION("A simple IRC bouncer using an IRC connection to a single IRC server on a listening port");
MODULE_VERSION(0, 1, 7);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 5), MODULE_DEPENDENCY("irc_channel", 0, 1, 4), MODULE_DEPENDENCY("irc", 0, 2, 7), MODULE_DEPENDENCY("irc_proxy", 0, 1, 6), MODULE_DEPENDENCY("config", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

HOOK_LISTENER(bouncer_reattach);
static IrcConnection *irc;
static IrcProxy *proxy;

MODULE_INIT
{
	Store *config = $(Store *, config, getConfigPathValue)("irc");

	if(config == NULL) {
		return false;
	}

	if((irc = $(IrcConnection *, irc, createIrcConnectionByStore)(config)) == NULL) {
		LOG_ERROR("Failed to establich remote IRC connection, aborting IRC bouncer");
		return false;
	}

	if(!$(bool, irc_channel, enableChannelTracking)(irc)) {
		LOG_ERROR("Failed to enable channel tracking for remote IRC connection %d, aborting IRC bouncer", irc->socket->fd);
		$(void, irc, freeIrcConnection)(irc);
		return false;
	}

	LOG_INFO("Successfully established remote IRC connection for IRC bouncer");

	Store *bouncer = $(Store *, config, getConfigPathValue)("irc/bouncer");

	if(bouncer == NULL) {
		return false;
	}

	Store *param;
	char *port;
	char *password;

	if((param = $(Store *, config, getStorePath)(bouncer, "port")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'port', aborting IRC bouncer");
		return false;
	}

	port = param->content.string;

	if((param = $(Store *, config, getStorePath)(bouncer, "password")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'password', aborting IRC bouncer");
		return false;
	}

	password = param->content.string;

	if((proxy = $(IrcProxy *, irc_proxy, createIrcProxy)(irc, port, password)) == NULL) {
		LOG_ERROR("Failed to create IRC proxy on port %s", port);
		return false;
	}

	// Enable plugin support
	if(!$(bool, irc_proxy_plugin, enableIrcProxyPlugins)(proxy)) {
		$(void, irc_proxy, freeIrcProxy)(proxy);
		$(void, irc, freeIrcConnection)(irc);
		LOG_ERROR("Failed to enable IRC proxy plugins for IRC bouncer, aborting");
		return false;
	}

	// Enable plugins listed in config / params
	if((param = $(Store *, config, getStorePath)(bouncer, "plugins")) != NULL && param->type == STORE_LIST) {
		GQueue *plugins = param->content.list;
		unsigned int i = 0;

		for(GList *iter = plugins->head; iter != NULL; iter = iter->next, i++) {
			Store *pentry = iter->data;

			if(pentry->type != STORE_STRING) {
				$(void, irc_proxy, freeIrcProxy)(proxy);
				$(void, irc, freeIrcConnection)(irc);
				LOG_ERROR("Element %d of param list 'plugins' for IRC bouncer is not a string but of store type %d, aborting", i, pentry->type);
				return false;
			}

			char *pname = pentry->content.string;

			if(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, pname)) {
				LOG_WARNING("Failed to enable perform plugin %s for IRC bouncer, skipping", pname);
			}
		}
	}

	HOOK_ATTACH(irc_proxy_client_authenticated, bouncer_reattach);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_proxy_client_authenticated, bouncer_reattach);

	$(void, irc_proxy_plugins, disableIrcProxyPlugins)(proxy); // disable plugins of the proxy
	$(void, irc_proxy, freeIrcProxy)(proxy); // disable the proxy itself
	$(void, irc, freeIrcConnection)(irc); // free the remote connection
}

HOOK_LISTENER(bouncer_reattach)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);

	GList *channels = $(GList *, irc_channel, getTrackedChannels)(irc);

	for(GList *iter = channels; iter != NULL; iter = iter->next) {
		IrcChannel *channel = iter->data;
		$(bool, irc_proxy, proxyClientIrcSend)(client, ":%s!%s@%s JOIN %s", irc->nick, irc->user, irc->socket->host, channel->name);
		$(bool, irc, ircSend)(irc, "NAMES %s", channel->name);
		$(bool, irc, ircSend)(irc, "TOPIC %s", channel->name);
	}

	g_list_free(channels);
}
