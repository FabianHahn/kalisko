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
#include "modules/irc/irc.h"
#include "api.h"
#include "irc_channel.h"

MODULE_NAME("irc_channel");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The IRC channel module keeps track of channel joins and leaves as well as of their users");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 3, 2));

HOOK_LISTENER(irc_line);

static GHashTable *tracked;

MODULE_INIT
{
	tracked = g_hash_table_new(NULL, NULL);

	HOOK_ATTACH(irc_line, irc_line);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(irc_line, irc_line);
	g_hash_table_destroy(tracked);
}

HOOK_LISTENER(irc_line)
{

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
	tracker->channels = g_hash_table_new(&g_str_hash, &g_str_equal);

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
}
