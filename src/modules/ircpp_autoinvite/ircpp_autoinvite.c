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
#include "timer.h"
#include "modules/config/config.h"
#include "modules/irc/irc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#include "modules/event/event.h"
#define API

MODULE_NAME("ircpp_autoinvite");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin automatically joins channels to which you are invited");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("socket", 0, 4, 4), MODULE_DEPENDENCY("irc", 0, 5, 0), MODULE_DEPENDENCY("irc_proxy", 0, 3, 0), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 1), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_remoteLine(void *subject, const char *event, void *data, va_list args);
static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);

static IrcProxyPlugin plugin;

MODULE_INIT
{
	plugin.name = "autoinvite";
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

static void listener_remoteLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	IrcProxy *proxy;
	if((proxy = getIrcProxyByIrcConnection(irc)) != NULL) { // this IRC connection is actually proxied
		if(isIrcProxyPluginEnabled(proxy, "autoinvite")) { // plugin is enabled for this proxy
			if(g_strcmp0(message->command, "INVITE") == 0) { // we got an invite!
				if(message->params_count > 1 && g_strcmp0(irc->nick, message->params[0]) == 0 && message->params[1] != NULL) { // it's a valid invite
					char *channel = message->params[1];
					logNotice("Received invite to channel '%s', autojoining...", channel);
					ircSend(irc, "JOIN %s", channel);
				}
			}
		}
	}
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
	attachEventListener(proxy->irc, "line", NULL, &listener_remoteLine);

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
	detachEventListener(proxy->irc, "line", NULL, &listener_remoteLine);
}

