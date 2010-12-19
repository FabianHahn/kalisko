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

static Store *xcall_parseIrcMessage(Store *xcall);
static Store *xcall_parseIrcUserMask(Store *xcall);

MODULE_NAME("xcall_irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall Module for irc_parser");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("xcall", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("parseIrcMessage", &xcall_parseIrcMessage)) {
		return false;
	}

	if(!$(bool, xcall, addXCallFunction)("parseIrcUserMask", &xcall_parseIrcUserMask)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("parseIrcMessage");
	$(bool, xcall, delXCallFunction)("parseIrcUserMask");
}

/**
 * XCall function for parseIrcMessage of irc_parser module.
 *
 * XCall parameters:
 * 	* string 'message'		the IRC message line to parse
 *
 * XCall result on success:
 *  * integer 'success' = 1
 * 	* array 'ircMessage':	an array representing a valid IRC message
 *    * string 'prefix'			the prefix part of the IRC message
 *    * string 'command'		the command part of the IRC message
 *    * string 'trailing'		the trailing part of the IRC message
 *    * string 'raw_message'	the original IRC message as it was given to the parser
 *    * integer 'params_count'	count of IRC params
 *    * list 'params'			list of string values representing IRC parameters
 *
 * XCall result on failure:
 *  * integer 'success' = 0
 *  * array 'error':		an array with informations about an error
 *    * string 'id'			an identifier to identify the error
 *    * string 'message'	a message what went wrong.
 *
 * @see IrcMessage
 * @see parseIrcMessage
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_parseIrcMessage(Store *xcall)
{
	Store *message = $(Store *, store, getStorePath)(xcall, "message");
	Store *ret = $(Store *, store, createStore)();

	if(message == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));

		return ret;
	}

	IrcMessage *ircMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message->content.string);
	if(ircMessage == NULL) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "error", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "error/id", $(Store *, store, createStoreStringValue)("irc_parser.irc_message.parse_not_possible"));
		$(bool, store, setStorePath)(ret, "error/message", $(Store *, store, createStoreStringValue)("Given IRC message cannot be parsed."));

		return ret;
	}

	$(bool, store, setStorePath)(ret, "ircMessage", $(Store *, store, createStoreArrayValue)(NULL));
	if(ircMessage->prefix) {
		$(bool, store, setStorePath)(ret, "ircMessage/prefix", $(Store *, store, createStoreStringValue)(ircMessage->prefix));
	}

	if(ircMessage->command) {
		$(bool, store, setStorePath)(ret, "ircMessage/command", $(Store *, store, createStoreStringValue)(ircMessage->command));
	}

	if(ircMessage->trailing) {
		$(bool, store, setStorePath)(ret, "ircMessage/trailing", $(Store *, store, createStoreStringValue)(ircMessage->trailing));
	}

	if(ircMessage->raw_message) {
		$(bool, store, setStorePath)(ret, "ircMessage/raw_message", $(Store *, store, createStoreStringValue)(ircMessage->raw_message));
	}

	$(bool, store, setStorePath)(ret, "ircMessage/params_count", $(Store *, store, createStoreIntegerValue)(ircMessage->params_count));

	GQueue *paramQueue = g_queue_new();
	for(int i = 0; i < ircMessage->params_count; i++) {
		g_queue_push_tail(paramQueue, $(Store *, store, createStoreStringValue)(ircMessage->params[i]));
	}
	$(bool, store, setStorePath)(ret, "ircMessage/params", $(Store *, store, createStoreListValue)(paramQueue));

	$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(1));

	$(void, irc_parser, freeIrcMessage)(ircMessage);

	return ret;
}

/**
 * XCall function for parseIrcUserMask of irc_parser module.
 *
 * XCall parameters:
 * 	* string 'prefix'		the IRC prefix containing user mask
 *
 * XCall result on success:
 *  * integer 'success' = 1
 * 	* array 'ircUserMask':	an array representing an IRC user mask
 *    * string 'host'	the host part of the user mask
 *    * string 'nick'	the nick part of the user mask
 *    * string 'user'	the user part of the user mask
 *
 * XCall result on failure:
 *  * integer 'success' = 0
 *  * array 'error':		an array with informations about an error
 *    * string 'id'			an identifier to identify the error
 *    * string 'message'	a message what went wrong.
 *
 * @see IrcUserMask
 * @see parseIrcUserMask
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_parseIrcUserMask(Store *xcall)
{
	Store *prefix = $(Store *, store, getStorePath)(xcall, "prefix");
	Store *ret = $(Store *, store, createStore)();

	if(prefix == NULL || prefix->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'prefix'"));

		return ret;
	}

	IrcUserMask *userMask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(prefix->content.string);
	if(userMask == NULL) {
		$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(ret, "error", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "error/id", $(Store *, store, createStoreStringValue)("irc_parser.irc_user_mask.parse_not_possible"));
		$(bool, store, setStorePath)(ret, "error/message", $(Store *, store, createStoreStringValue)("Given IRC user mask cannot be parsed."));

		return ret;
	}

	$(bool, store, setStorePath)(ret, "ircUserMask", $(Store *, store, createStoreArrayValue)(NULL));

	if(userMask->host) {
		$(bool, store, setStorePath)(ret, "ircUserMask/host", $(Store *, store, createStoreStringValue)(userMask->host));
	}

	if(userMask->nick) {
		$(bool, store, setStorePath)(ret, "ircUserMask/nick", $(Store *, store, createStoreStringValue)(userMask->nick));
	}

	if(userMask->user) {
		$(bool, store, setStorePath)(ret, "ircUserMask/user", $(Store *, store, createStoreStringValue)(userMask->user));
	}

	$(void, irc_parser, freeIrcUserMask)(userMask);

	return ret;
}
