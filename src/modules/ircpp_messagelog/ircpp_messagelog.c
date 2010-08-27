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
#include <stdio.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/string_util/string_util.h"
#include "api.h"

MODULE_NAME("ircpp_messagelog");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that allows IRC messages to be logged to the hard drive");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 5), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 2), MODULE_DEPENDENCY("irc_parser", 0, 1, 4), MODULE_DEPENDENCY("string_util", 0, 1, 3));

HOOK_LISTENER(client_line);
HOOK_LISTENER(remote_line);

static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);
static IrcProxyPlugin plugin;
static char *messagelog_folder = ".";

MODULE_INIT
{
	plugin.name = "messagelog";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	HOOK_ATTACH(irc_proxy_client_line, client_line);
	HOOK_ATTACH(irc_line, remote_line);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_proxy_client_line, client_line);
	HOOK_DETACH(irc_line, remote_line);

	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);
}

HOOK_LISTENER(client_line)
{
	IrcProxyClient *client = HOOK_ARG(IrcProxyClient *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);

	if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, "messagelog")) { // plugin is enabled for this proxy
		if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 && !$(bool, irc_proxy, hasIrcProxyRelayException)(client->proxy, message->params[0]) && message->trailing != NULL) {
			GString *path = g_string_new(messagelog_folder);
			g_string_append_printf(path, "/%s", client->proxy->name);

			if(!g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
				if(g_mkdir_with_parents(path->str, 0750) == -1) {
					LOG_SYSTEM_ERROR("Failed to create IRC proxy message log folder %s", path->str);
					g_string_free(path, true);
					return;
				}
			}

			char *target = g_ascii_strdown(message->params[0], strlen(message->params[0]));
			$(void, string_util, convertToFilename)(target);
			g_string_append_printf(path, "/%s.log", target);
			free(target);

			FILE *file;

			if((file = fopen(path->str, "a")) == NULL) {
				LOG_SYSTEM_ERROR("Failed to open IRC proxy message log file %s", path->str);
				g_string_free(path, true);
				return;
			}

			GTimeVal now;
			g_get_current_time(&now);

			char *nowstr = g_time_val_to_iso8601(&now);
			fprintf(file, "[%s] <%s> %s\n", nowstr, client->proxy->irc->nick, message->trailing);

			free(nowstr);
			fclose(file);
			g_string_free(path, true);
		}
	}
}

HOOK_LISTENER(remote_line)
{
	IrcConnection *irc = HOOK_ARG(IrcConnection *);
	IrcMessage *message = HOOK_ARG(IrcMessage *);
	IrcProxy *proxy;

	if((proxy = $(IrcProxy *, irc_proxy, getIrcProxyByIrcConnection)(irc)) != NULL) {
		if($(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(proxy, "messagelog")) { // plugin is enabled for this proxy
			if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 && !$(bool, irc_proxy, hasIrcProxyRelayException)(proxy, message->params[0]) && message->trailing != NULL) {
				GString *path = g_string_new(messagelog_folder);
				g_string_append_printf(path, "/%s", proxy->name);

				if(!g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
					if(g_mkdir_with_parents(path->str, 0750) == -1) {
						LOG_SYSTEM_ERROR("Failed to create IRC proxy message log folder %s", path->str);
						g_string_free(path, true);
						return;
					}
				}

				IrcUserMask *mask;

				if((mask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix)) == NULL) {
					g_string_free(path, true);
					return;
				}

				char *target;
				if(g_strcmp0(message->params[0], proxy->irc->nick) == 0) { // no channel message, query!
					target = g_ascii_strdown(mask->nick, strlen(mask->nick));
				} else {
					target = g_ascii_strdown(message->params[0], strlen(message->params[0]));
				}

				$(void, string_util, convertToFilename)(target);
				g_string_append_printf(path, "/%s.log", target);
				free(target);

				FILE *file;

				if((file = fopen(path->str, "a")) == NULL) {
					LOG_SYSTEM_ERROR("Failed to open IRC proxy message log file %s", path->str);
					g_string_free(path, true);
					$(void, irc_parser, freeIrcUserMask)(mask);
					return;
				}

				GTimeVal now;
				g_get_current_time(&now);

				char *nowstr = g_time_val_to_iso8601(&now);
				fprintf(file, "[%s] <%s> %s\n", nowstr, mask->nick, message->trailing);

				free(nowstr);
				fclose(file);
				g_string_free(path, true);
				$(void, irc_parser, freeIrcUserMask)(mask);
			}
		}
	}
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

}
