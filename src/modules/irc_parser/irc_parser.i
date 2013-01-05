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
	 *
	 * If the prefix is a user mask this var contains the user's nick name. It's the irc message part before '!'.
	 */
	char *nick;

	/**
	 * (optional) The user part if the prefix is a user mask. It's the irc message part after '!' and before '@'.
	 */
	char *user;

	/**
	 * (optional) The host of the user if the prefix is a user mask. It's the irc message part after '@'.
	 */
	char *host;
} IrcUserMask;

/**
 * Represents an irc message as defined in RFC 1459.
 */
typedef struct {
	/**
	 * The prefix of a irc message is optional. If set it contains the server name or the user mask.
	 *
	 * The prefix is the part of an irc message which you pass to parseIrcUserMask.
	 *
	 * @par Example messages
	 * Example message:
	 * @code EU.GameSurge.net PRIVMSG #kalisko :Hello World @endcode
	 * Prefix value:
	 * @code EU.GameSurge.net @endcode @n
	 * Example message:
	 * @code :Someone!someone@Someone.user.gamesurge PRIVMSG #php.de :Hello World @endcode
	 * Prefix value:
	 * @code Someone!someone@Someone.user.gamesurge @endcode
	 */
	char *prefix;

	/**
	 * The command is the part between the prefix and the trailing whitespace.
	 *
	 * @par Example messages
	 * Example message:
	 * @code PING :EU.GameSurge.net @endcode
	 * Command value:
	 * @code PING @endcode @n
	 * Example message:
	 * @code NOTICE AUTH :*** Looking up your hostname @endcode
	 * Command value:
	 * @code NOTICE @endcode
	 * Information: 'AUTH' is a param as between 'NOTICE' and 'AUTH' is a whitespace
	 */
	char *command;

	/**
	 * NULL-terminated array of parameter strings.
	 *
	 * An irc message parameter is between the command and a ':'.
	 *
	 * @par Example messages
	 * Example message:
	 * @code NOTICE AUTH :*** Looking up your hostname @endcode
	 * Params values: @n
	 * [0]
	 * @code AUTH @endcode
	 * [1]
	 * @code NULL @endcode
	 */
	char **params;
	
	/**
	 * Amount of given parameters (in params)
	 */
	unsigned int params_count;

	/**
	 * The trailing part is everything after the ':'.
	 *
	 * @par Example messages
	 * Example message:
	 * @code NOTICE AUTH :*** Looking up your hostname @endcode
	 * Trailing value:
	 * @code *** Looking up your hostname @endcode @n
	 * Example value:
	 * @code PING :EU.GameSurge.net @endcode
	 * Trailing value:
	 * @code EU.GameSurge.net @endcode
	 */
	char *trailing;

	/**
	 * The original unchanged irc message.
	 */
	char *raw_message;
} IrcMessage;


/**
 * This function parses an IRC message as described in RFC 1459 (Chapter 2.3.1).
 *
 * @param message	An IRC message. Has to be on heap.
 * @return 			A struct containing the different parts of the message. If an error occured NULL is returned.
 */
API IrcMessage *parseIrcMessage(char *message);

/**
 * Parses the prefix part of an IRC message to extract the different parts of a user mask. The returned user mask
 * could be the server name (as the nick) and not a real user mask as there is no way to determine what the prefix is exactly.
 *
 * See for further information RFC 1459.
 *
 * @param prefix	The prefix part of an IRC message.
 * @return 			A struct containing the different parts of a user mask or NULL if an error occurred.
 */
API IrcUserMask *parseIrcUserMask(char *prefix);

/**
 * Frees the given IrcMessage.
 *
 * @param message	The IrcMessage to free.
 */
API void freeIrcMessage(IrcMessage *message);

/**
 * Frees the given IrcUserMask.
 *
 * @param userMask	The IrcUserMask to free.
 */
API void freeIrcUserMask(IrcUserMask *userMask);

#endif
