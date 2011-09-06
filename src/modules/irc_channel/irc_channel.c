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
#include "log.h"
#include "types.h"
#include "memory_alloc.h"
#include "modules/irc/irc.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/event/event.h"
#define API
#include "irc_channel.h"

MODULE_NAME("irc_channel");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC channel module keeps track of channel joins and leaves as well as of their users");
MODULE_VERSION(0, 1, 8);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 3, 2), MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("event", 0, 1, 2));

static void listener_ircLine(void *subject, const char *event, void *data, va_list args);
static void listener_ircDisconnect(void *subject, const char *event, void *data, va_list args);
static void freeIrcChannel(void *channel_p);
static bool freeIrcChannelTracker(void *key_p, void *tracker_p, void *data);

/**
 * A hash table to associate IrcConnection objects with IrcChannelTracker objects
 */
static GHashTable *tracked;

MODULE_INIT
{
	tracked = g_hash_table_new(NULL, NULL);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_foreach_remove(tracked, &freeIrcChannelTracker, NULL);
	g_hash_table_destroy(tracked);
}

static void listener_ircLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	IrcUserMask *mask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix);
	if(mask == NULL) {
		return;
	}

	IrcChannelTracker *tracker;
	IrcChannel *channel;
	if((tracker = g_hash_table_lookup(tracked, irc)) != NULL) {
		if(g_strcmp0(message->command, "JOIN") == 0 && (message->params_count >= 1 || message->trailing != NULL)) {
			if(g_strcmp0(irc->nick, mask->nick) == 0) { // Its ourselves!
				char *cname = NULL;
				if(message->params_count >= 1) {
					cname = message->params[0];
				} else {
					cname = message->trailing;
				}

				// Construct new channel
				channel = ALLOCATE_OBJECT(IrcChannel);
				channel->name = strdup(cname);
				channel->tracker = tracker;
				g_hash_table_insert(tracker->channels, channel->name, channel); // add it to channels table

				LOG_DEBUG("Joined channel %s on IRC conection %d", channel->name, irc->socket->fd);

				$(int, event, triggerEvent)(irc, "channel_join", channel);
			}
		} else if(g_strcmp0(message->command, "PART") == 0 && (message->params_count >= 1 || message->trailing != NULL)) {
			if(g_strcmp0(irc->nick, mask->nick) == 0) { // Its ourselves!
				char *cname = NULL;
				if(message->params_count >= 1) {
					cname = message->params[0];
				} else {
					cname = message->trailing;
				}

				if((channel = g_hash_table_lookup(tracker->channels, cname)) != NULL) {
					g_hash_table_remove(tracker->channels, cname);
					LOG_DEBUG("Left channel %s on IRC connection %d", cname, irc->socket->fd);
					$(int, event, triggerEvent)(irc, "channel_part", cname);
				}
			}
		}

		free(mask);
	}
}

static void listener_ircDisconnect(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *irc = subject;

	IrcChannelTracker *tracker;
	if((tracker = g_hash_table_lookup(tracked, irc)) != NULL) { // we are tracking channels for this connection
		g_hash_table_remove_all(tracker->channels); // clear channel list if we disconnected
	}
}


/**
 * Enables channel tracking for an IRC connection
 *
 * @param irc		the IRC connection to enable tracking for
 * @result			true if successful
 */
API bool enableChannelTracking(IrcConnection *irc)
{
	IrcChannelTracker *tracker;

	if((tracker = g_hash_table_lookup(tracked, irc)) != NULL) {
		LOG_WARNING("Trying to enable channel tracking for already tracked IRC connection %d, aborting", irc->socket->fd);
		return false;
	}

	tracker = ALLOCATE_OBJECT(IrcChannelTracker);
	tracker->irc = irc;
	tracker->channels = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeIrcChannel);

	g_hash_table_insert(tracked, irc, tracker);

	$(void, event, attachEventListener)(irc, "line", NULL, &listener_ircLine);
	$(void, event, attachEventListener)(irc, "disconnect", NULL, &listener_ircDisconnect);

	return true;
}

/**
 * Disables channel tracking for an IRC connection
 *
 * @param irc		the IRC connection to disable tracking for
 */
API void disableChannelTracking(IrcConnection *irc)
{
	IrcChannelTracker *tracker;

	if((tracker = g_hash_table_lookup(tracked, irc)) == NULL) {
		LOG_WARNING("Trying to disable channel tracking for untracked IRC connection %d, aborting", irc->socket->fd);
		return;
	}

	g_hash_table_destroy(tracker->channels);
	free(tracker);

	$(void, event, detachEventListener)(irc, "line", NULL, &listener_ircLine);
	$(void, event, detachEventListener)(irc, "disconnect", NULL, &listener_ircDisconnect);

	g_hash_table_remove(tracked, irc);
}

/**
 * Retrieves a tracked IRC channel from a tracked IRC connection
 *
 * @param irc		the IRC connection that tracks the channels
 * @param name		the name of the channel to retrieve
 * @result			the IRC channel or NULL if that channel is not tracked
 */
API IrcChannel *getTrackedChannel(IrcConnection *irc, char *name)
{
	IrcChannelTracker *tracker;

	if((tracker = g_hash_table_lookup(tracked, irc)) == NULL) {
		LOG_WARNING("Trying to retrieve channel %s for untracked IRC connection %d, aborting", name, irc->socket->fd);
		return NULL;
	}

	return g_hash_table_lookup(tracker->channels, name);
}

/**
 * Retrieves all tracked IRC channels from a tracked IRC connection
 *
 * @param irc		the IRC connection that tracks the channels
 * @result			a list of tracked IRC channels, must be freed with g_list_free but not be modified
 */
API GList *getTrackedChannels(IrcConnection *irc)
{
	IrcChannelTracker *tracker;

	if((tracker = g_hash_table_lookup(tracked, irc)) == NULL) {
		LOG_WARNING("Trying to retrieve tracked channels for untracked IRC connection %d, aborting", irc->socket->fd);
		return NULL; // NULL is the empty list, that's fine!
	}

	return g_hash_table_get_values(tracker->channels);
}

/**
 * A GDestroyNotify function to free an IRC channel
 *
 * @param channel_p			a pointer to the IRC channel to be freed
 */
static void freeIrcChannel(void *channel_p)
{
	IrcChannel *channel = channel_p;

	free(channel->name);
	free(channel);
}

/**
 * A GHRFunc to free an IRC channel tracker
 *
 * @param key_p			unused
 * @param tracker_p		a pointer to the tracker to free
 * @param data			unused
 */
static bool freeIrcChannelTracker(void *key_p, void *tracker_p, void *data)
{
	IrcChannelTracker *tracker = tracker_p;

	g_hash_table_destroy(tracker->channels);
	free(tracker);

	return true; // always free the tracker in the parent hashtable
}
