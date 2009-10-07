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


#ifndef IRC_PARSER_IRC_PARSER_H
#define IRC_PARSER_IRC_PARSER_H

/**
 * Represents an IRC user mask.
 */
typedef struct {
	/**
	 * The nick of the user but could also be the server name as it is not possible to determine if the
	 * IRC message prefix is a user mask or the server name.
	 */
	char *nick;
	char *user;
	char *host;
} IrcUserMask;

/**
 * Represents an IRC message as defined in RFC 1459.
 */
typedef struct {
	char *prefix;
	char *command;

	/**
	 * NULL terminated array of parameter strings.
	 */
	char **params;
	char *trailing;

	/**
	 * The original not changed IRC message.
	 */
	char *ircMessage;
} IrcMessage;

API IrcMessage *parseIrcMessage(char *message);
API IrcUserMask *parseIrcUserMask(char *prefix);
API void freeIrcMessage(IrcMessage *message);
API void freeIrcUserMask(IrcUserMask *userMask);

#endif
