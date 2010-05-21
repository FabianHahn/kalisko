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

#ifndef IRC_IRC_H
#define IRC_IRC_H

#include <glib.h>
#include "modules/store/store.h"
#include "modules/socket/socket.h"

/**
 * Struct to represent an IRC connection
 */
typedef struct
{
	/** the connection password to use */
	char *password;
	/** the user name to use */
	char *user;
	/** the real name to use */
	char *real;
	/** the nick to use */
	char *nick;
	/** input buffer for IRC messages */
	GString *ibuffer;
	/** true if IRC output should be throttled */
	bool throttle;
	/** output buffer for IRC messages */
	GQueue *obuffer;
	/** the time used for throttling */
	double throttle_time;
	/** socket of the IRC connection, also stores host and port of the connection */
	Socket *socket;
} IrcConnection;

API IrcConnection *createIrcConnection(char *server, char *port, char *password, char *user, char *real, char *nick);
API IrcConnection *createIrcConnectionByStore(Store *cfg);
API bool enableIrcConnectionThrottle(IrcConnection *irc);
API void disableIrcConnectionThrottle(IrcConnection *irc, bool flush_output_buffer);
API void freeIrcConnection(IrcConnection *irc);
API bool ircSend(IrcConnection *irc, char *message, ...) G_GNUC_PRINTF(2, 3);
API void authenticateIrcConnection(IrcConnection *irc);
API IrcConnection *getIrcConnectionBySocket(Socket *socket);

/**
 * The maximal length for a log message.
 */
#define IRC_SEND_MAXLEN 4096

#endif
