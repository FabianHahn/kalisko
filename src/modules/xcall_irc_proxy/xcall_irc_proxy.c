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
#include "types.h"
#include "memory_alloc.h"
#include "util.h"
#include "log.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#include "modules/xcall/xcall.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"

#include "api.h"

static Store *xcall_proxyClientIrcSend(Store *xcall);

MODULE_NAME("xcall_irc_proxy");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for irc_proxy");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 0), MODULE_DEPENDENCY("xcall", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("socket", 0, 5, 1));

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("proxyClientIrcSend", &xcall_proxyClientIrcSend)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("proxyClientIrcSend");
}

/**
 * XCallFunction to send a message to an IRC proxy client
 * XCall parameters:
 *  * int client		the socket fd of the client proxy client
 *  * string message	the message to send to the IRC proxy client
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_proxyClientIrcSend(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	Store *message;
	Store *clientId;
	Socket *socket;
	IrcProxyClient *client;

	if((message = $(Store *, store, getStorePath)(xcall, "message")) == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));
		return ret;
	}

	if((clientId = $(Store *, store, getStorePath)(xcall, "client")) == NULL || clientId->type != STORE_INTEGER) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory integer parameter 'client'"));
		return ret;
	}

	if((socket = $(Socket *, socket, getPolledSocketByFd)(clientId->content.integer)) == NULL) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		return ret;
	}

	if((client = $(IrcProxyClient *, irc_proxy, getIrcProxyClientBySocket)(socket)) == NULL) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		return ret;
	}

	$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)($(bool, irc_proxy, proxyClientIrcSend)(client, "%s", message->content.string)));

	return ret;
}
