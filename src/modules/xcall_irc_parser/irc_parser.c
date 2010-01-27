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
#include "modules/irc_parser/irc_parser.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#include "modules/xcall/xcall.h"

#include "api.h"

static GString *parseIrcMessageXCall(const char *xcall);
static GString *parseIrcUserMaskXCall(const char *xcall);

MODULE_NAME("xcall_irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall Module for irc_parser");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 6, 0));

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("parseIrcMessage", &parseIrcMessageXCall)) {
		return false;
	}

	if(!$(bool, xcall, addXCallFunction)("parseIrcUserMask", &parseIrcUserMaskXCall)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("parseIrcMessage");
	$(bool, xcall, delXCallFunction)("parseIrcUserMask");
}

static GString *parseIrcMessageXCall(const char *xcall)
{
	Store *call = $(Store *, store, parseStoreString)(xcall);
	Store *message = $(Store *, store, getStorePath)(call, "message");

	if(message == NULL || message->type != STORE_STRING) {
		$(void, store, freeStore(call));
		// error
		return NULL;
	}

	IrcMessage *ircMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message->content.string);
	if(ircMessage == NULL) {
		$(void, store, freeStore(call));
		// error
		return NULL;
	}

	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "ircMessage", $(Store *, store, createStoreArrayValue)(NULL));
	if(ircMessage->prefix) {
		$(bool, store, setStorePath)(ret, "ircMessage/prefix", $(Store *, store, createStoreStringValue)(strdup(ircMessage->prefix)));
	}

	if(ircMessage->command) {
		$(bool, store, setStorePath)(ret, "ircMessage/command", $(Store *, store, createStoreStringValue)(strdup(ircMessage->command)));
	}

	if(ircMessage->trailing) {
		$(bool, store, setStorePath)(ret, "ircMessage/trailing", $(Store *, store, createStoreStringValue)(strdup(ircMessage->trailing)));
	}

	if(ircMessage->raw_message) {
		$(bool, store, setStorePath)(ret, "ircMessage/raw_message", $(Store *, store, createStoreStringValue)(strdup(ircMessage->raw_message)));
	}

	$(bool, store, setStorePath)(ret, "ircMessage/params_count", $(Store *, store, createStoreIntegerValue)(ircMessage->params_count));

	GQueue *paramQueue = g_queue_new();
	for(int i = 0; i <= ircMessage->params_count; i++) {
		if(ircMessage->params[i]) {
			g_queue_push_tail(paramQueue, $(Store *, store, createStoreStringValue)(strdup(ircMessage->params[i])));
		}
	}
	$(bool, store, setStorePath)(ret, "ircMessage/params", $(Store *, store, createStoreListValue)(paramQueue));

	$(void, irc_parser, freeIrcMessage)(ircMessage);

	GString *retStr = $(GString *, store, writeStoreGString)(ret);
	$(void, store, freeStore(ret));

	return  retStr;
}

static GString *parseIrcUserMaskXCall(const char *xcall)
{
	Store *call = $(Store *, store, parseStoreString)(xcall);
	Store *prefix = $(Store *, store, getStorePath)(call, "prefix");

	if(prefix == NULL || prefix->type != STORE_STRING) {
		$(void, store, freeStore)(call);
		// error
		return NULL;
	}

	IrcUserMask *userMask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(prefix->content.string);
	if(userMask == NULL) {
		$(void, store, freeStore)(call);
		// error
		return NULL;
	}

	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "ircUserMask", $(Store *, store, createStoreArrayValue)(NULL));

	if(userMask->host) {
		$(bool, store, setStorePath)(ret, "ircUserMask/host", $(Store *, store, createStoreStringValue)(strdup(userMask->host)));
	}

	if(userMask->nick) {
		$(bool, store, setStorePath)(ret, "ircUserMask/nick", $(Store *, store, createStoreStringValue)(strdup(userMask->nick)));
	}

	if(userMask->user) {
		$(bool, store, setStorePath)(ret, "ircUserMask/user", $(Store *, store, createStoreStringValue)(strdup(userMask->user)));
	}

	$(void, irc_parser, freeIrcUserMask)(userMask);

	GString *retStr = $(GString *, store, writeStoreGString)(ret);
	$(void, store, freeStore(ret));

	return retStr;
}
