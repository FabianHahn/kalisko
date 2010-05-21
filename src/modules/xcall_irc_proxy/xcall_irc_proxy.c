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
static Store *xcall_getIrcProxyClients(Store *xcall);
static Store *xcall_getIrcProxies(Store *xcall);

MODULE_NAME("xcall_irc_proxy");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for irc_proxy");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy", 0, 3, 4), MODULE_DEPENDENCY("xcall", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("socket", 0, 5, 1));

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("proxyClientIrcSend", &xcall_proxyClientIrcSend)) {
		return false;
	}

	if(!$(bool, xcall, addXCallFunction)("getIrcProxyClients", &xcall_getIrcProxyClients)) {
		return false;
	}

	if(!$(bool, xcall, addXCallFunction)("getIrcProxies", &xcall_getIrcProxies)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("proxyClientIrcSend");
	$(bool, xcall, delXCallFunction)("getIrcProxyClients");
	$(bool, xcall, delXCallFunction)("getIrcProxies");
}

/**
 * XCallFunction to retrieve all IRC proxy clients for an IRC proxy
 * XCall parameters:
 *  * int client		the socket fd of the client proxy client
 *  * string message	the message to send to the IRC proxy client
 * XCall result:
 * 	* list success		nonzero if successful
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

/**
 * XCallFunction to send a message to an IRC proxy client
 * XCall parameters:
 *  * string proxy		the proxy to retrieve clients for
 * XCall result:
 * 	* list clients		an integer list of clients for this IRC proxy
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getIrcProxyClients(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	Store *name;
	IrcProxy *proxy;

	if((name = $(Store *, store, getStorePath)(xcall, "proxy")) == NULL || name->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'proxy'"));
		return ret;
	}

	if((proxy = $(IrcProxy *, irc_proxy, getIrcProxyByName)(name->content.string)) == NULL) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		return ret;
	}

	GQueue *clients = g_queue_new();

	// Loop over all IRC proxy clients for this IRC proxy
	for(GList *iter = proxy->clients->head; iter != NULL; iter = iter->next) {
		IrcProxyClient *client = iter->data;
		g_queue_push_tail(clients, $(Store *, store, createStoreIntegerValue)(client->socket->fd));
	}

	$(bool, store, setStorePath)(ret, "clients", $(Store *, store, createStoreListValue)(clients));

	return ret;
}

/**
 * XCallFunction retrieve all IRC proxies
 * XCall result:
 * 	* list proxies		a string list of all available IRC proxies
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getIrcProxies(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();


	GList *proxyList = $(GList *, irc_proxy, getIrcProxies)();
	GQueue *proxies = g_queue_new();

	// Loop over all IRC proxies
	for(GList *iter = proxyList; iter != NULL; iter = iter->next) {
		IrcProxy *proxy = iter->data;
		g_queue_push_tail(proxies, $(Store *, store, createStoreStringValue)(proxy->name));
	}

	$(bool, store, setStorePath)(ret, "proxies", $(Store *, store, createStoreListValue)(proxies));

	g_list_free(proxyList);

	return ret;
}

