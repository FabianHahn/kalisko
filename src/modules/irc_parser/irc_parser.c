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
#include <string.h>

#include "dll.h"
#include "types.h"
#include "memory_alloc.h"
#include "util.h"
#include "log.h"
#include "modules/string_util/string_util.h"

#include "api.h"
#include "irc_parser.h"

MODULE_NAME("irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Parses and creates IRC messages");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("string_util", 0, 1, 0));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{
}

/**
 * This function parses an IRC message as described in RFC 1459 (Chapter 2.3.1).
 *
 * @param message	An IRC message. Has to be on heap.
 * @return 			A struct containing the different parts of the message. If an error occured NULL is returned.
 */
API IrcMessage *parseIrcMessage(char *message)
{
	IrcMessage *ircMessage = ALLOCATE_OBJECT(IrcMessage);
	memset(ircMessage, 0, sizeof(IrcMessage));

	char *msg = g_strdup(message);
	ircMessage->raw_message = g_strdup(message);

	char *prefixEnd = msg;

	if(msg[0] == ':') {
		// message has a prefix
		prefixEnd = strchr(msg, ' ');
		if(prefixEnd == NULL) {
			LOG_ERROR("Malformed IRC message: '%s'", msg);
			freeIrcMessage(ircMessage);
			return NULL;
		}

		msg++; // Get rid of trailing colon
		ircMessage->prefix = g_strndup(msg, prefixEnd - msg);
	}

	// extracting the command
	g_strchug(prefixEnd);
	char *commandEnd = strchr(prefixEnd, ' ');
	if(commandEnd == NULL) { // just a command
		ircMessage->command = g_strdup(prefixEnd);
		return ircMessage;
	}

	ircMessage->command = g_strndup(prefixEnd, commandEnd - prefixEnd);

	// extracting the params
	char *paramsEnd = strchr(commandEnd, ':');

	char *paramsText = NULL;
	if(paramsEnd == NULL) {
		paramsText = g_strdup(commandEnd);
	} else {
		int paramsTextLength = paramsEnd - commandEnd - 1; // Remove trailing colon

		if(paramsTextLength > 0) { // It is possible that the IRC message does not have a parameter (like PING)
			paramsText = g_strndup(commandEnd, paramsTextLength);
		}
	}

	if(paramsText) {
		g_strstrip(paramsText);
		$(void, string_util, stripDuplicateWhitespace)(paramsText);

		ircMessage->params = g_strsplit(paramsText, " ", 0);
		// I haven't found any glib function doing this, so I'll count manually
		for(ircMessage->params_count = 0; ircMessage->params[ircMessage->params_count] != NULL; ircMessage->params_count++) {
			// Nothing left to do
		}

		free(paramsText);
	}

	// extracting trailing
	if(paramsEnd) {
		g_strchomp(paramsEnd);
		ircMessage->trailing = g_strdup(paramsEnd + 1); // Once again, remove trailing colon
	}

	return ircMessage;
}

/**
 * Parses the prefix part of an IRC message to extract the different parts of a user mask. The returned user mask
 * could be the server name (as the nick) and not a real user mask as there is no way to determine what the prefix is exactly.
 *
 * See for further information RFC 1459.
 *
 * @param prefix	The prefix part of an IRC message.
 * @return 			A struct containing the different parts of a user mask or NULL if an error occurred.
 */
API IrcUserMask *parseIrcUserMask(char *prefix)
{
	if(!prefix) {
		return NULL;
	}

	IrcUserMask *mask = ALLOCATE_OBJECT(IrcUserMask);
	memset(mask, 0, sizeof(IrcUserMask));

	char *userStart = strchr(prefix, '!');
	char *hostStart = strchr(prefix, '@');

	if(!userStart && !hostStart) {
		mask->nick = g_strdup(prefix);
	} else {
		mask->nick = g_strndup(prefix, userStart - prefix);

		if(userStart && hostStart) {
			mask->user = g_strndup(userStart + 1, (hostStart - userStart) - 1);
			mask->host = g_strdup(hostStart + 1);
		} else { // !hostStart || !userStart
			if(!hostStart) {
				mask->user = g_strdup(userStart + 1);
			} else if(!userStart) {
				mask->host = g_strdup(hostStart + 1);
			}
		}
	}

	return mask;
}

/**
 * Frees the given IrcMessage.
 *
 * @param message	The IrcMessage to free.
 */
API void freeIrcMessage(IrcMessage *message)
{
	if(message->command) {
		free(message->command);
	}

	if(message->raw_message) {
		free(message->raw_message);
	}

	if(message->params) {
		g_strfreev(message->params);
	}

	if(message->prefix) {
		free(message->prefix);
	}

	if(message->trailing) {
		free(message->trailing);
	}

	free(message);
}

/**
 * Frees the given IrcUserMask.
 *
 * @param userMask	The IrcUserMask to free.
 */
API void freeIrcUserMask(IrcUserMask *userMask)
{
	if(userMask->host) {
		free(userMask->host);
	}

	if(userMask->nick) {
		free(userMask->nick);
	}

	if(userMask->user) {
		free(userMask->user);
	}

	free(userMask);
}
