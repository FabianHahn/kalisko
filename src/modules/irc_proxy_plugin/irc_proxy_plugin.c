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

#include "api.h"
#include "irc_proxy_plugin.h"


MODULE_NAME("irc_proxy_plugin");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC proxy plugin module manages manages plugins that can be activated and deactivated for individual IRC proxies");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 1, 13));

static GHashTable *proxies;
static GHashTable *plugins;

MODULE_INIT
{
	proxies = g_hash_table_new(NULL, NULL);
	plugins = g_hash_table_new(NULL, NULL);

	return true;
}

MODULE_FINALIZE
{
	// Since all plugins depend on this module, we don't need to free the contents
	g_hash_table_destroy(plugins);
	g_hash_table_destroy(proxies);
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
	g_hash_table_remove(plugins, plugin->name);
}
