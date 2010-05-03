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
#include "memory_alloc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "api.h"
#include "irc_proxy_plugin.h"


MODULE_NAME("irc_proxy_plugin");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC proxy plugin module manages manages plugins that can be activated and deactivated for individual IRC proxies");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13));

static bool unloadIrcProxyPlugin(void *key_p, void *plugin_p, void *handler_p);
/**
 * Hash table to associate IrcProxy objects to IrcProxyHandler objects
 */
static GHashTable *handlers;
/**
 * Hash table to associate plugin names (char *) to IrcProxyPlugin objects
 */
static GHashTable *plugins;

MODULE_INIT
{
	handlers = g_hash_table_new(NULL, NULL);
	plugins = g_hash_table_new(&g_str_hash, &g_str_equal);

	return true;
}

MODULE_FINALIZE
{
	// Since all plugins depend on this module, we don't need to free the contents
	g_hash_table_destroy(plugins);
	g_hash_table_destroy(handlers);
}

/**
 * Adds an IRC proxy plugin to the plugins pool
 *
 * @param plugin		the plugin to add
 * @result				true if successful
 */
API bool addIrcProxyPlugin(IrcProxyPlugin *plugin)
{
	if(g_hash_table_lookup(plugins, plugin->name) != NULL) {
		LOG_ERROR("Trying to enable already enabled IRC proxy plugin %s, aborting", plugin->name);
		return false;
	}

	g_hash_table_insert(plugins, plugin->name, plugin);

	return true;
}

/**
 * Removes an IRC proxy plugin from the plugins pool
 *
 * @param plugin		the plugin to remove
 */
API void delIrcProxyPlugin(IrcProxyPlugin *plugin)
{
	GQueue *list = g_queue_new();

	// Build a list of all IRC proxy plugin handlers from which we must be unloaded
	for(GList *iter = plugin->handlers->head; iter != NULL; iter = iter->next) {
		g_queue_push_head(list, iter->data);
	}

	// Loop through the list and remove the plugin from each handler
	for(GList *iter = list->head; iter != NULL; iter = iter->next) {
		IrcProxyPluginHandler *handler;
		unloadIrcProxyPlugin(NULL, plugin, handler); // unload the plugin for this handler
		g_hash_table_remove(handler->plugins, plugin->name); // remove the plugin from the handler's plugin table
	}

	g_queue_free(list); // remove our temp queue

	// Also clear the list of plugins after unloading
	g_queue_clear(plugin->handlers);

	g_hash_table_remove(plugins, plugin->name);
}

/**
 * Enables plugin support for an IRC proxy
 *
 * @param proxy			the IRC proxy to enable plugins for
 * @result				true if successful
 */
API bool enableIrcProxyPlugins(IrcProxy *proxy)
{
	IrcProxyPluginHandler *handler;

	if((handler = g_hash_table_lookup(handlers, proxy)) != NULL) {
		LOG_ERROR("Trying to enable IRC proxy plugins for already enabled IRC proxy on port %s", proxy->server->port);
		return false;
	}

	handler = ALLOCATE_OBJECT(IrcProxyPluginHandler);
	handler->proxy = proxy;
	handler->plugins = g_hash_table_new(&g_str_hash, &g_str_equal);

	g_hash_table_insert(handlers, proxy, handler);

	return true;
}

/**
 * Disable plugin support for an IRC proxy
 *
 * @param proxy			the IRC proxy to disable plugins for
 */
API void disableIrcProxyPlugins(IrcProxy *proxy)
{
	IrcProxyPluginHandler *handler;

	if((handler = g_hash_table_lookup(handlers, proxy)) == NULL) { // already disabled, do nothing
		return;
	}

	g_hash_table_foreach_remove(handler->plugins, &unloadIrcProxyPlugin, handler); // unload all plugins for this proxy
	g_hash_table_destroy(handler->plugins); // destroy their table
	free(handler); // also free the plugins handler
}

/**
 * Enable an IRC proxy plugin for a specific proxy
 *
 * @param proxy			the proxy to enable the plugin for
 * @param name			the name of the plugin to enable
 * @result				true if successful
 */
API bool enableIrcProxyPlugin(IrcProxy *proxy, char *name)
{
	IrcProxyPluginHandler *handler;

	// Lookup the handler for this proxy
	if((handler = g_hash_table_lookup(handlers, proxy)) == NULL) {
		LOG_ERROR("Trying to enable IRC proxy plugin %s for IRC proxy on port %s without plugins enabled, aborting", name, proxy->server->port);
		return false;
	}

	IrcProxyPlugin *plugin;

	// Lookup the plugin for the given plugin name
	if((plugin = g_hash_table_lookup(plugins, name)) == NULL) {
		LOG_ERROR("Trying to enable non existing proxy plugin %s for IRC proxy on port %s, aborting", name, proxy->server->port);
		return false;
	}

	// TODO: actually load the plugin

	g_hash_table_insert(handler->plugins, plugin->name, plugin);
	g_queue_push_head(plugin->handlers, handler);

	LOG_INFO("Enabled IRC proxy plugin %s for IRC proxy on port %s", name, proxy->server->port);

	return true;
}

/**
 * Disables an IRC proxy plugin for a specific proxy
 *
 * @param proxy			the proxy to enable the plugin for
 * @param name			the name of the plugin to enable
 * @result				true if successful
 */
API bool disableIrcProxyPlugin(IrcProxy *proxy, char *name)
{
	IrcProxyPluginHandler *handler;

	// Lookup the handler for this proxy
	if((handler = g_hash_table_lookup(handlers, proxy)) == NULL) {
		LOG_ERROR("Trying to disable IRC proxy plugin %s for IRC proxy on port %s without plugins enabled, aborting", name, proxy->server->port);
		return false;
	}

	IrcProxyPlugin *plugin;

	// Lookup the plugin for the given plugin name
	if((plugin = g_hash_table_lookup(plugins, name)) == NULL) {
		LOG_ERROR("Trying to disable non existing proxy plugin %s for IRC proxy on port %s, aborting", name, proxy->server->port);
		return false;
	}

	if(g_hash_table_lookup(handler->plugins, name) != plugin) {
		LOG_ERROR("Trying to disable non enabled IRC proxy plugin %s for IRC proxy on port %s, aborting", name, proxy->server->port);
		return false;
	}

	if(!unloadIrcProxyPlugin(NULL, plugin, handler)) {
		LOG_ERROR("Failed to disable IRC proxy plugin %s for IRC proxy on port %s", name, proxy->server->port);
		return false;
	}

	LOG_INFO("Disabled IRC proxy plugin %s for IRC proxy on port %s", name, proxy->server->port);

	return true;
}

/**
 * A GHRFunc to unload an IRC proxy plugin from an IRC proxy. Note that this doesn't remove the plugin from the parent handler's plugins table
 *
 * @param key_p			unused
 * @param plugin_p		a pointer to the IRC proxy plugin to unload
 * @param handler_p		a pointer to the handler to free the plugin for
 * @result				always true (the plugin should be removed from the underlying hash table)
 */
static bool unloadIrcProxyPlugin(void *key_p, void *plugin_p, void *handler_p)
{
	IrcProxyPlugin *plugin = plugin_p;
	IrcProxyPluginHandler *handler = handler_p;

	// TODO: actually unload the plugin

	g_queue_remove(plugin->handlers, handler);

	return true;
}
