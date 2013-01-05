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


/**
 * Creates an IRC connection. Note that this function doesn't block and returns immediately, the "reconnect" event is triggered once the connection
 * is establishes and the client is authenticated.
 *
 * @param server		IRC server to connect to
 * @param port			IRC server's port to connect to
 * @param user			user name to use
 * @param password		password to use
 * @param real			real name to use
 * @param nick			nick to use
 * @result				the created IRC connection or NULL on failure
 */
API IrcConnection *createIrcConnection(char *server, char *port, char *password, char *user, char *real, char *nick);

/**
 * Creates an IRC connection by reading the required parameters from a store
 *
 * @param params		the store to read the parameters from
 * @result				the created IRC connection or NULL on failure
 */
API IrcConnection *createIrcConnectionByStore(Store *cfg);

/**
 * Reconnects an IRC connection. Note that this function doesn't block and returns immediately, the actual reconnection is done inside a timer and the "reconnect" event
 * is sent once the reconnection actually worked.
 *
 * @param irc		the IRC connection to reconnect
 * @result			true if the reconnection was successfully attempted
 */
API bool reconnectIrcConnection(IrcConnection *irc);

/**
 * Enables output throttling for an IRC connection
 *
 * @param irc		the IRC connection to enable output throttling for
 * @result			true if successful
 */
API bool enableIrcConnectionThrottle(IrcConnection *irc);

/**
 * Disables output throttling for an IRC connection
 *
 * @param irc					the IRC connection to disable output throttling for
 * @param flush_output_buffer	if true, the output buffer is flushed before freeing, i.e. all remaining buffered messages will be burst-sent to the server
 */
API void disableIrcConnectionThrottle(IrcConnection *irc, bool flush_output_buffer);

/**
 * Frees an IRC connection
 *
 * @param irc		the IRC connection to free
 */
API void freeIrcConnection(IrcConnection *irc);

/**
 * Sends a message to the IRC connection's remote server
 *
 * @param irc			the IRC connection to use for sending the message
 * @param message		printf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool ircSend(IrcConnection *irc, char *message, ...) G_GNUC_PRINTF(2, 3);

/**
 * Sends a message to the IRC connection's remote server. If the connection is throttled, makes sure the message is sent before all already queued messages
 *
 * @param irc			the IRC connection to use for sending the message
 * @param message		printf-style message to send to the socket
 * @result				true if successful, false on error
 */
API bool ircSendFirst(IrcConnection *irc, char *message, ...) G_GNUC_PRINTF(2, 3);

/**
 * Authenticate an IRC connection by sending USER, NICK and PASS lines
 *
 * @param irc		the IRC connection to authenticate
 */
API void authenticateIrcConnection(IrcConnection *irc);

/**
 * Retrieves an IRC connection by its socket
 *
 * @param socket		the socket to look up
 * @result				the IRC connection, or NULL if none was found for this socket
 */
API IrcConnection *getIrcConnectionBySocket(Socket *socket);

/**
 * The maximal length for a log message.
 */
#define IRC_SEND_MAXLEN 4096

#endif
