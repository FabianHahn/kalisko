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
#include "test.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"

#define API

MODULE_NAME("test_xcall_irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the xcall_irc_parser module");
MODULE_VERSION(0, 1, 6);
MODULE_BCVERSION(0, 1, 6);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("xcall_irc_parser", 0, 2, 0));

TEST(xcall_irc_parse);
TEST(xcall_irc_parse_user_mask);
TEST(xcall_irc_parse_error);
TEST(xcall_irc_parse_no_message);
TEST(xcall_irc_parse_user_mask_no_prefix);

TEST_SUITE_BEGIN(xcall_irc_parser)
	ADD_SIMPLE_TEST(xcall_irc_parse);
	ADD_SIMPLE_TEST(xcall_irc_parse_user_mask);
	ADD_SIMPLE_TEST(xcall_irc_parse_error);
	ADD_SIMPLE_TEST(xcall_irc_parse_no_message);
	ADD_SIMPLE_TEST(xcall_irc_parse_user_mask_no_prefix);
TEST_SUITE_END

TEST(xcall_irc_parse)
{
	Store *retStore = $(Store *, xcall, invokeXCallByString)("message = \":irc.gamesurge.net            366           Gregor          @         #php.de         :    Do         something!\r\n\"; xcall = { function = \"parseIrcMessage\" }");

	TEST_ASSERT(getStorePath(retStore, "success") != NULL);
	TEST_ASSERT(getStorePath(retStore, "success")->content.integer == 1);

	TEST_ASSERT(getStorePath(retStore, "ircMessage/prefix") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircMessage/prefix")->content.string, "irc.gamesurge.net") == 0);

	TEST_ASSERT(getStorePath(retStore, "ircMessage/command") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircMessage/command")->content.string, "366") == 0);

	TEST_ASSERT(getStorePath(retStore, "ircMessage/params_count") != NULL);
	TEST_ASSERT(getStorePath(retStore, "ircMessage/params_count")->content.integer == 3);

	TEST_ASSERT(getStorePath(retStore, "ircMessage/params/0") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircMessage/params/0")->content.string, "Gregor") == 0);

	$(void, store, freeStore)(retStore);
}

TEST(xcall_irc_parse_user_mask)
{
	Store *retStore = $(Store *, xcall, invokeXCallByString)("prefix = \"Gregor!kalisko@kalisko.org\"; xcall = { function = \"parseIrcUserMask\" }");

	TEST_ASSERT(getStorePath(retStore, "ircUserMask/nick") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircUserMask/nick")->content.string, "Gregor") == 0);

	TEST_ASSERT(getStorePath(retStore, "ircUserMask/user") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircUserMask/user")->content.string, "kalisko") == 0);

	TEST_ASSERT(getStorePath(retStore, "ircUserMask/host") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "ircUserMask/host")->content.string, "kalisko.org") == 0);

	$(void, store, freeStore)(retStore);
}


TEST(xcall_irc_parse_error)
{
	Store *retStore = $(Store *, xcall, invokeXCallByString)("message = \":nothing\"; xcall = { function = \"parseIrcMessage\" }");

	TEST_ASSERT(getStorePath(retStore, "success") != NULL);
	TEST_ASSERT(getStorePath(retStore, "success")->content.integer == 0);

	TEST_ASSERT(getStorePath(retStore, "error/id") != NULL);
	TEST_ASSERT(strcmp(getStorePath(retStore, "error/id")->content.string, "irc_parser.irc_message.parse_not_possible") == 0);

	$(void, store, freeStore)(retStore);
}

TEST(xcall_irc_parse_no_message)
{
	Store *retStore = $(Store *, xcall, invokeXCallByString)("xcall = { function = \"parseIrcMessage\" }");

	TEST_ASSERT(getStorePath(retStore, "success") != NULL);
	TEST_ASSERT(getStorePath(retStore, "success")->content.integer == 0);

	$(void, store, freeStore)(retStore);
}

TEST(xcall_irc_parse_user_mask_no_prefix)
{
	Store *retStore = $(Store *, xcall, invokeXCallByString)("xcall = { function = \"parseIrcUserMask\" }");

	TEST_ASSERT(getStorePath(retStore, "success") != NULL);
	TEST_ASSERT(getStorePath(retStore, "success")->content.integer == 0);

	$(void, store, freeStore)(retStore);
}
