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
#include "modules/irc/irc.h"
#include "api.h"
#include "irc_proxy.h"

MODULE_NAME("irc_proxy");
MODULE_AUTHOR("smf68");
MODULE_DESCRIPTION("The IRC proxy module relays IRC traffic from and to an IRC server through a server socket");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 2, 6), MODULE_DEPENDENCY("socket", 0, 3, 0));

HOOK_LISTENER(remote_line);
HOOK_LISTENER(client_accept);
HOOK_LISTENER(client_read);
HOOK_LISTENER(client_disconnect);

MODULE_INIT
{
	HOOK_ATTACH(irc_line, remote_line);
	HOOK_ATTACH(socket_accept, client_accept);
	HOOK_ATTACH(socket_read, client_read);
	HOOK_ATTACH(socket_disconnect, client_disconnect);

	return true;
}

MODULE_FINALIZE
{

}

HOOK_LISTENER(remote_line)
{

}

HOOK_LISTENER(client_accept)
{

}

HOOK_LISTENER(client_read)
{

}

HOOK_LISTENER(client_disconnect)
{

}

/**
 * Creates an IRC proxy relaying data for an IRC connection
 *
 * @param irc			the IRC connection to relay
 * @param port			the server port to listen on for client connections
 * @param password		password to use for client connections
 * @result				the created IRC proxy, or NULL on failure
 */
API IrcProxy *createIrcProxy(IrcConnection *irc, char *port, char *password)
{
	return NULL;
}
