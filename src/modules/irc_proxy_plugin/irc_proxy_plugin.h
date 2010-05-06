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


#ifndef IRC_PROXY_PLUGIN_IRC_PROXY_PLUGIN
#define IRC_PROXY_PLUGIN_IRC_PROXY_PLUGIN

#include <glib.h>
#include "modules/irc_proxy/irc_proxy.h"

typedef struct {
	/** the IRC proxy to handle plugins for */
	IrcProxy *proxy;
	/** a table that associated plugin names (char *) with IrcProxyPlugin objects loaded for this IRC proxy */
	GHashTable *plugins;
} IrcProxyPluginHandler;

typedef struct {
	/** the name of the plugin */
	char *name;
	/** a list of IrcProxyPluginHandler objects that have this plugin activated */
	GQueue *handlers;
} IrcProxyPlugin;

API bool addIrcProxyPlugin(IrcProxyPlugin *plugin);
API void delIrcProxyPlugin(IrcProxyPlugin *plugin);
API bool enableIrcProxyPlugins(IrcProxy *proxy);
API void disableIrcProxyPlugins(IrcProxy *proxy);
API bool enableIrcProxyPlugin(IrcProxy *proxy, char *name);
API bool isIrcProxyPluginEnabled(IrcProxy *proxy, char *name);
API bool disableIrcProxyPlugin(IrcProxy *proxy, char *name);

#endif
