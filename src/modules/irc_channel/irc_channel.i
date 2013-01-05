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


#ifndef IRC_CHANNEL_IRC_CHANNEL_H
#define IRC_CHANNEL_IRC_CHANNEL_H

#include <glib.h>
#include "modules/irc/irc.h"

typedef struct {
	/** the IRC connection that is tracked */
	IrcConnection *irc;
	/** a table of channels being tracked for the connection */
	GHashTable *channels;
} IrcChannelTracker;

typedef struct {
	/** the responsible tracker for this channel */
	IrcChannelTracker *tracker;
	/** the name of the channel */
	char *name;
} IrcChannel;


/**
 * Enables channel tracking for an IRC connection
 *
 * @param irc		the IRC connection to enable tracking for
 * @result			true if successful
 */
API bool enableChannelTracking(IrcConnection *irc);

/**
 * Disables channel tracking for an IRC connection
 *
 * @param irc		the IRC connection to disable tracking for
 */
API void disableChannelTracking(IrcConnection *irc);

/**
 * Retrieves a tracked IRC channel from a tracked IRC connection
 *
 * @param irc		the IRC connection that tracks the channels
 * @param name		the name of the channel to retrieve
 * @result			the IRC channel or NULL if that channel is not tracked
 */
API IrcChannel *getTrackedChannel(IrcConnection *irc, char *name);

/**
 * Retrieves all tracked IRC channels from a tracked IRC connection
 *
 * @param irc		the IRC connection that tracks the channels
 * @result			a list of tracked IRC channels, must be freed with g_list_free but not be modified
 */
API GList *getTrackedChannels(IrcConnection *irc);

#endif
