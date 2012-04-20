/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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
#include "log.h"
#include "types.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/string_util/string_util.h"
#include "modules/event/event.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#define API

MODULE_NAME("ircpp_messagebuffer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("An IRC proxy plugin that sends the last few lines to new connected clients");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 5), MODULE_DEPENDENCY("irc_proxy_plugin", 0, 2, 2), MODULE_DEPENDENCY("irc_parser", 0, 1, 4), MODULE_DEPENDENCY("string_util", 0, 1, 3), MODULE_DEPENDENCY("event", 0, 1, 2), MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("store", 0, 5, 3));

/**
 * Each proxy has (if this module is used) his own instance of ProxyBuffer.
 */
typedef struct {
	/**
	 * A HashTable with the channel/query name as the key and a Queue as the value.
	 */
	GHashTable *chansBuffer;

	/**
	 * Contains the value of how many lines should be saved per channel/query.
	 * The key is the name of the channel/query and the value an integer with the
	 * maximum value.
	 */
	GHashTable *chanMaxLines;

	/**
	 * Contains the default value for how many lines should be saved.
	 */
	int defaultMaxLines;
} ProxyBuffer;

#define MESSAGEBUF_MAX_LINES 50

static bool initPlugin(IrcProxy *proxy, char *name);
static void finiPlugin(IrcProxy *proxy, char *name);

static void listener_remoteLine(void *subject, const char *event, void *data, va_list args);
static void listener_clientReattached(void *subject, const char *event, void *data, va_list args);
static void listener_clientLine(void *subject, const char *event, void *data, va_list args);


static IrcProxyPlugin plugin;

/**
 * A HashTable with the name of the proxy as the key and a reference
 * to ProxyBuffer as the value.
 */
static GHashTable *buffers;

MODULE_INIT
{
	plugin.name = "messagebuffer";
	plugin.handlers = g_queue_new();
	plugin.initialize = &initPlugin;
	plugin.finalize = &finiPlugin;

	if(!$(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin)) {
		return false;
	}

	buffers = g_hash_table_new(g_str_hash, g_str_equal);

	return true;
}

MODULE_FINALIZE
{
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);
}

/**
 * Handles IRC messages sent from the BNC's client
 *
 * @param subject
 * @param event
 * @param data
 * @param args
 */
static void listener_clientLine(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if(!$(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(client->proxy, "messagebuffer")) {
		return;
	}

	// check if we wanna save the message
	if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 &&
		!$(bool, irc_proxy, hasIrcProxyRelayException)(client->proxy, message->params[0]) &&
		message->trailing != NULL) {

		ProxyBuffer *buffer = g_hash_table_lookup(buffers, client->proxy->name);

		// build string for buffer
		char *msgTarget = strdup(message->params[0]);

		// get the amount of lines we wanna save for the specific target
		int maxLines = buffer->defaultMaxLines;
		int *specificMaxLines = NULL;
		if((specificMaxLines = g_hash_table_lookup(buffer->chanMaxLines, msgTarget)) != NULL) {
			maxLines = GPOINTER_TO_INT(specificMaxLines);
		}

		// check if we can ignore the target
		if(maxLines <= 0) {
			return;
		}

		// date and time
		GDateTime *now = g_date_time_new_now_local();
		unsigned int day = g_date_time_get_day_of_month(now);
		unsigned int month = g_date_time_get_month(now);
		unsigned int year = g_date_time_get_year(now);
		unsigned int hour = g_date_time_get_hour(now);
		unsigned int minute = g_date_time_get_minute(now);
		unsigned int second = g_date_time_get_second(now);
	    g_date_time_unref(now);

		char *bufferStr = g_strdup_printf(":%s PRIVMSG %s :[%02u.%02u.%04u-%02u:%02u:%02u] %s", client->proxy->irc->nick, msgTarget, day, month, year, hour, minute, second, message->trailing);

		// create queue if needed
		GQueue *queue = NULL;
		if((queue = g_hash_table_lookup(buffer->chansBuffer, msgTarget)) == NULL) {
			queue = g_queue_new();
			g_hash_table_insert(buffer->chansBuffer, msgTarget, queue);
		}

		// add message to queue
		g_queue_push_tail(queue, bufferStr);

		// remove line if we have one too much
		if(queue->length > maxLines) {
			// as we only add one line we have to remove only one to meet the maxLines
			g_queue_pop_head(queue);
		}
	}
}

/**
 * Handles lines sent by the IRC server.
 *
 * @param subject
 * @param event
 * @param data
 * @param args
 */
static void listener_remoteLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	IrcProxy *proxy;
	if((proxy = $(IrcProxy *, irc_proxy, getIrcProxyByIrcConnection)(irc)) != NULL) {

		// check that the plugin is enabled
		if(!$(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(proxy, "messagebuffer")) {
			return;
		}

		// check if we wanna save the message
		if(g_strcmp0(message->command, "PRIVMSG") == 0 && message->params_count > 0 &&
			!$(bool, irc_proxy, hasIrcProxyRelayException)(proxy, message->params[0]) &&
			message->trailing != NULL) {

			ProxyBuffer *buffer = g_hash_table_lookup(buffers, proxy->name);

			// get the user mask for the message
			IrcUserMask *usrMask;
			if((usrMask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix)) == NULL) {
				return;
			}

			// filter out the target of the message
			char *msgTarget;
			if(g_strcmp0(message->params[0], proxy->irc->nick) == 0) {
				// it is a query
				msgTarget = strdup(usrMask->nick);
			} else {
				// it is a channel
				msgTarget = strdup(message->params[0]);
			}

			// get the amount of lines we wanna save for the specific target
			int maxLines = buffer->defaultMaxLines;
			int *specificMaxLines = NULL;
			if((specificMaxLines = g_hash_table_lookup(buffer->chanMaxLines, msgTarget)) != NULL) {
				maxLines = GPOINTER_TO_INT(specificMaxLines);
			}

			// check if we can ignore the target
			if(maxLines <= 0) {
				return;
			}

			// date and time
			GDateTime *now = g_date_time_new_now_local();
			unsigned int day = g_date_time_get_day_of_month(now);
			unsigned int month = g_date_time_get_month(now);
			unsigned int year = g_date_time_get_year(now);
			unsigned int hour = g_date_time_get_hour(now);
			unsigned int minute = g_date_time_get_minute(now);
			unsigned int second = g_date_time_get_second(now);
		    g_date_time_unref(now);

			// build string for buffer
			char *bufferStr = g_strdup_printf(":%s PRIVMSG %s :[%02u.%02u.%04u-%02u:%02u:%02u] %s", message->prefix, msgTarget, day, month, year, hour, minute, second, message->trailing);

			// create queue if needed
			GQueue *queue = NULL;
			if((queue = g_hash_table_lookup(buffer->chansBuffer, msgTarget)) == NULL) {
				queue = g_queue_new();
				g_hash_table_insert(buffer->chansBuffer, msgTarget, queue);
			}

			// add message to queue
			g_queue_push_tail(queue, bufferStr);

			// remove line if we have one too much
			if(queue->length > maxLines) {
				// as we only add one line we have to remove only one to meet the maxLines
				g_queue_pop_head(queue);
			}

			// clean up
			$(void, irc_parser, freeIrcUserMask)(usrMask);
		}
	}
}

/**
 * Sends the buffer to a new BNC client
 * @param subject
 * @param event
 * @param data
 * @param args
 */
static void listener_clientReattached(void *subject, const char *event, void *data, va_list args)
{
	IrcProxy *proxy = subject;
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	// check that the plugin is enabled
	if(!$(bool, irc_proxy_plugin, isIrcProxyPluginEnabled)(proxy, "messagebuffer")) {
		return;
	}

	// send the queues
	ProxyBuffer *buffer = g_hash_table_lookup(buffers, proxy->name);

	// send buffers to new client
	GHashTableIter iter;
	void *key = NULL;
	void *value = NULL;

	g_hash_table_iter_init(&iter, buffer->chansBuffer);
	while(g_hash_table_iter_next(&iter, &key, &value)) {
		char *target = key;
		GQueue *messages = value;

		if(messages->length > 0) {
			char *infoSender = NULL;
			if(g_str_has_prefix(target, "#")) {
				infoSender = "*messagebuffer!kalisko@kalisko.org";
			} else {
				infoSender = target;
			}

			$(bool, irc_proxy, proxyClientIrcSend)(client, ":%s PRIVMSG %s :Message buffer playback...", infoSender, target);
			char *message = NULL;
			while((message = g_queue_pop_head(messages)) != NULL) {
				$(bool, irc_proxy, proxyClientIrcSend)(client, "%s", message);
			}
			$(bool, irc_proxy, proxyClientIrcSend)(client, ":%s PRIVMSG %s :...buffer playback complete!", infoSender, target);
		}
	}
}

static void listener_clientAuthenticated(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	$(void, event, attachEventListener)(client, "line", NULL, &listener_clientLine);
}

static void listener_clientDisconnected(void *subject, const char *event, void *data, va_list args)
{
	IrcProxyClient *client = va_arg(args, IrcProxyClient *);

	$(void, event, detachEventListener)(client, "line", NULL, &listener_clientLine);
}

static bool initPlugin(IrcProxy *proxy, char *name)
{
	// Attach to existing clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		$(void, event, attachEventListener)(client, "line", NULL, &listener_clientLine);
	}

	// create new ProxyBuffer for given proxy
	if(g_hash_table_lookup(buffers, proxy->name) == NULL) {
		ProxyBuffer *buffer = ALLOCATE_OBJECT(ProxyBuffer);

		buffer->chansBuffer = g_hash_table_new(g_str_hash, g_str_equal);
		buffer->chanMaxLines = g_hash_table_new(g_str_hash, g_str_equal);
		buffer->defaultMaxLines = MESSAGEBUF_MAX_LINES;

		// read settings from config
		Store *config = NULL;
		char *storePath = g_build_path("/", "irc/bouncers", proxy->name, "messagebuffer", NULL);
		if((config = $(Store *, config, getConfigPath)(storePath)) != NULL) {
			// irc/bouncers/maxLines
			Store *maxLinesConfig = NULL;
			if((maxLinesConfig = $(Store *, store, getStorePath)(config, "maxLines")) != NULL) {
				if(maxLinesConfig->type == STORE_INTEGER) {
					buffer->defaultMaxLines = maxLinesConfig->content.integer;
				} else {
					LOG_INFO("Found 'maxLines' setting but it is not an Integer. Using internal default.");
					buffer->defaultMaxLines = MESSAGEBUF_MAX_LINES;
				}
			}

			// irc/bouncers/specific
			Store *specificLinesConfig = NULL;
			if((specificLinesConfig = $(Store *, store, getStorePath)(config, "specific")) != NULL) {
				if(specificLinesConfig->type == STORE_ARRAY) {
					GHashTableIter specificIter;
					char *key;
					Store *value;

					g_hash_table_iter_init(&specificIter, specificLinesConfig->content.array);
					while(g_hash_table_iter_next(&specificIter, (void *)&key, (void *)&value)) {
						if(value->type == STORE_INTEGER) {
							g_hash_table_insert(buffer->chanMaxLines, strdup(key), GINT_TO_POINTER(value->content.integer));
						} else {
							LOG_INFO("Found setting for '%s' but the value is not an Integer. Ignoring.", key);
						}
					}
				} else {
					LOG_INFO("Found 'specific' setting but it is not an Array. Ignoring.");
				}
			}
		} else {
			LOG_INFO("No config for ircpp_messagebuffer found.");
		}

		// clean up
		free(storePath);
		g_hash_table_insert(buffers, proxy->name, buffer);
	}

	$(void, event, attachEventListener)(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	$(void, event, attachEventListener)(proxy, "client_disconnected", NULL, &listener_clientDisconnected);
	$(void, event, attachEventListener)(proxy, "bouncer_reattached", NULL, &listener_clientReattached);
	$(void, event, attachEventListener)(proxy->irc, "line", NULL, &listener_remoteLine);

	return true;
}

static void finiPlugin(IrcProxy *proxy, char *name)
{
	$(void, event, detachEventListener)(proxy, "client_authenticated", NULL, &listener_clientAuthenticated);
	$(void, event, detachEventListener)(proxy, "client_disconnected", NULL, &listener_clientDisconnected);
	$(void, event, detachEventListener)(proxy, "bouncer_reattached", NULL, &listener_clientReattached);
	$(void, event, detachEventListener)(proxy->irc, "line", NULL, &listener_remoteLine);

	// Detach from remaining clients
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		$(void, event, detachEventListener)(client, "line", NULL, &listener_clientLine);
	}

	ProxyBuffer *buffer = NULL;
	if((buffer = g_hash_table_lookup(buffers, proxy->name)) != NULL) {
		g_hash_table_remove(buffers, proxy->name);

		GHashTableIter iter;
		void *key;
		void *value;

		// clean up 'chansBuffer'
		g_hash_table_iter_init(&iter, buffer->chansBuffer);
		while(g_hash_table_iter_next(&iter, &key, &value)) {
			GQueue *queue = value;
			for(GList *iter = queue->head; iter != NULL; iter = iter->next) {
				free(iter->data);
			}

			free(key);
			g_queue_free(queue);
		}

		g_hash_table_destroy(buffer->chansBuffer);

		// clean up 'chanMaxLines'
		g_hash_table_iter_init(&iter, buffer->chanMaxLines);
		while(g_hash_table_iter_next(&iter, &key, &value)) {
			free(key);
		}

		g_hash_table_destroy(buffer->chanMaxLines);

		free(buffer);
	}
}
