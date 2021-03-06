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

#define API
#include "irc_parser.h"

MODULE_NAME("irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Parses and creates IRC messages");
MODULE_VERSION(0, 1, 4);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("string_util", 0, 1, 0));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{
}

API IrcMessage *parseIrcMessage(char *message)
{
	IrcMessage *ircMessage = ALLOCATE_OBJECT(IrcMessage);
	memset(ircMessage, 0, sizeof(IrcMessage));

	ircMessage->raw_message = g_strdup(message);

	char *prefixEnd = message;

	if(message[0] == ':') {
		// message has a prefix
		prefixEnd = strchr(message, ' ');
		if(prefixEnd == NULL) {
			logError("Malformed IRC message: '%s'", message);
			freeIrcMessage(ircMessage);
			return NULL;
		}

		message++; // Get rid of trailing colon
		ircMessage->prefix = g_strndup(message, prefixEnd - message);
	}

	// extracting the command
	char *commandPart = g_strdup(prefixEnd);
	g_strchug(commandPart);
	char *commandEnd = strchr(commandPart, ' ');
	if(commandEnd == NULL) { // just a command
		ircMessage->command = g_strdup(commandPart);

		free(commandPart);
		return ircMessage;
	}

	ircMessage->command = g_strndup(commandPart, commandEnd - commandPart);

	// extracting the params
	char *paramsEnd = strstr(commandEnd, " :");

	char *paramsText = NULL;
	if(paramsEnd == NULL) {
		paramsText = g_strdup(commandEnd);
	} else {
		int paramsTextLength = paramsEnd - commandEnd;

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
		char *trailingPart = g_strdup(paramsEnd);
		g_strchomp(trailingPart);
		ircMessage->trailing = g_strdup(trailingPart + 2); // Remove whitespace & trailing colon

		free(trailingPart);
	}

	free(commandPart);

	return ircMessage;
}

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
