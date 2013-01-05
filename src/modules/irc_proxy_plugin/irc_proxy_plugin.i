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
#include "types.h"
#include "modules/irc_proxy/irc_proxy.h"

/**
 * Function pointer type to initialize an IRC proxy plugin
 */
typedef bool (IrcProxyPluginInitializer)(IrcProxy *proxy, char *plugin);

/**
 * Function pointer type to finalize an IRC proxy plugin
 */
typedef void (IrcProxyPluginFinalizer)(IrcProxy *proxy, char *plugin);

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
	/** the initializer function for this IRC proxy plugin */
	IrcProxyPluginInitializer *initialize;
	/** the finalizer function for this IRC proxy plugin */
	IrcProxyPluginFinalizer *finalize;
} IrcProxyPlugin;


/**
 * Adds an IRC proxy plugin to the plugins pool
 *
 * @param plugin		the plugin to add
 * @result				true if successful
 */
API bool addIrcProxyPlugin(IrcProxyPlugin *plugin);

/**
 * Removes an IRC proxy plugin from the plugins pool
 *
 * @param plugin		the plugin to remove
 */
API void delIrcProxyPlugin(IrcProxyPlugin *plugin);

/**
 * Enables plugin support for an IRC proxy
 *
 * @param proxy			the IRC proxy to enable plugins for
 * @result				true if successful
 */
API bool enableIrcProxyPlugins(IrcProxy *proxy);

/**
 * Disable plugin support for an IRC proxy
 *
 * @param proxy			the IRC proxy to disable plugins for
 */
API void disableIrcProxyPlugins(IrcProxy *proxy);

/**
 * Enable an IRC proxy plugin for a specific proxy
 *
 * @param proxy			the proxy to enable the plugin for
 * @param name			the name of the plugin to enable
 * @result				true if successful
 */
API bool enableIrcProxyPlugin(IrcProxy *proxy, char *name);

/**
 * Checks if an IRC proxy plugin is enabled for an IRC proxy
 *
 * @param proxy			the IRC proxy to check for the plugin
 * @param name			the IRC proxy plugin name to check
 * @result				true if the plugin with that name is loaded for proxy
 */
API bool isIrcProxyPluginEnabled(IrcProxy *proxy, char *name);

/**
 * Disables an IRC proxy plugin for a specific proxy
 *
 * @param proxy			the proxy to enable the plugin for
 * @param name			the name of the plugin to enable
 * @result				true if successful
 */
API bool disableIrcProxyPlugin(IrcProxy *proxy, char *name);

/**
 * Returns a list of available IRC proxy plugins
 *
 * @result			a list of all available plugins as strings. Must not be modified but freed with g_list_free after use
 */
API GList *getAvailableIrcProxyPlugins();

#endif
