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
#include "test.h"
#include "string.h"
#include "modules/irc_parser/irc_parser.h"

#define API

MODULE_NAME("test_irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the irc_parser module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_parser", 0, 1, 0));

TEST(utf8Trailing);
TEST(whitespaces);
TEST(userMask);
TEST(ping);
TEST(noticeAuth);
TEST(serverNotice);
TEST(onlyCommand);
TEST(passDelimiter);

TEST_SUITE_BEGIN(irc_parser)
	ADD_SIMPLE_TEST(utf8Trailing);
	ADD_SIMPLE_TEST(whitespaces);
	ADD_SIMPLE_TEST(userMask);
	ADD_SIMPLE_TEST(ping);
	ADD_SIMPLE_TEST(noticeAuth);
	ADD_SIMPLE_TEST(serverNotice);
	ADD_SIMPLE_TEST(onlyCommand);
	ADD_SIMPLE_TEST(passDelimiter);
TEST_SUITE_END

TEST(utf8Trailing)
{
	char *message = "Someone :Зарегистрируйтесь Unicode แผ่นดินฮั่นเสื่อมโทรมแสนสังเวช 1234567890 ╔══╦══╗  ┌──┬──┐  ╭──┬──╮  ╭──┬──╮\r\n";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);

	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(strcmp(parsedMessage->command, "Someone") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "Зарегистрируйтесь Unicode แผ่นดินฮั่นเสื่อมโทรมแสนสังเวช 1234567890 ╔══╦══╗  ┌──┬──┐  ╭──┬──╮  ╭──┬──╮") == 0);

	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(parsedMessage->params_count == 0);
	TEST_ASSERT(parsedMessage->prefix == NULL);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

TEST(whitespaces)
{
	char *message = ":irc.gamesurge.net            366           Gregor          @         #php.de         :    Do         something!\r\n";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);

	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(strcmp(parsedMessage->command, "366") == 0);
	TEST_ASSERT(strcmp(parsedMessage->prefix, "irc.gamesurge.net") == 0);

	TEST_ASSERT(strcmp(parsedMessage->params[0], "Gregor") == 0);
	TEST_ASSERT(strcmp(parsedMessage->params[1], "@") == 0);
	TEST_ASSERT(strcmp(parsedMessage->params[2], "#php.de") == 0);
	TEST_ASSERT(parsedMessage->params[3] == NULL);
	TEST_ASSERT(parsedMessage->params_count == 3);

	TEST_ASSERT(strcmp(parsedMessage->trailing, "    Do         something!") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

TEST(userMask)
{
	char *message = ":Gregor!kalisko@kalisko.org KICK #php.de Someone :blub";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);

	TEST_ASSERT(parsedMessage);

	IrcUserMask *userMask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(parsedMessage->prefix);

	TEST_ASSERT(strcmp(userMask->nick, "Gregor") == 0);
	TEST_ASSERT(strcmp(userMask->user, "kalisko") == 0);
	TEST_ASSERT(strcmp(userMask->host, "kalisko.org") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	$(void, irc_parser, freeIrcUserMask)(userMask);
}

TEST(ping)
{
	char *message = "PING :irc.gamesurge.net";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(strcmp(parsedMessage->command, "PING") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "irc.gamesurge.net") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

TEST(noticeAuth)
{
	char *message = "NOTICE AUTH :*** Looking up your hostname";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(strcmp(parsedMessage->params[0], "AUTH") == 0);
	TEST_ASSERT(parsedMessage->params[1] == NULL);
	TEST_ASSERT(parsedMessage->params_count == 1);
	TEST_ASSERT(strcmp(parsedMessage->command, "NOTICE") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "*** Looking up your hostname") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

TEST(serverNotice)
{
	char *message = ":Staff.CA.US.GameSurge.net NOTICE * :*** Notice -- Received KILL message for grog. From Someone Path: Someone.operator.support!Someone (.)";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(strcmp(parsedMessage->prefix, "Staff.CA.US.GameSurge.net") == 0);
	TEST_ASSERT(strcmp(parsedMessage->params[0], "*") == 0);
	TEST_ASSERT(parsedMessage->params[1] == NULL);
	TEST_ASSERT(parsedMessage->params_count == 1);
	TEST_ASSERT(strcmp(parsedMessage->command, "NOTICE") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "*** Notice -- Received KILL message for grog. From Someone Path: Someone.operator.support!Someone (.)") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

/**
 * Testcase for bug report #1406:
 * The irc_parser module currently fails to parse messages that have nothing but a command statement in them.
 * Practical examples include the commands "AWAY" and "QUIT" which may be sent without trailing content by clients.
 */
TEST(onlyCommand)
{
	char *message = "AWAY";

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(parsedMessage->params_count == 0);
	TEST_ASSERT(strcmp(parsedMessage->command, "AWAY") == 0);
	TEST_ASSERT(parsedMessage->trailing == NULL);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}

/**
 * Test case for bug ticket #1416: Parser doesn't handle colons in params correctly
 */
TEST(passDelimiter)
{
	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)("PASS user:password");
	TEST_ASSERT(parsedMessage != NULL);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(parsedMessage->params_count == 1);
	TEST_ASSERT(g_strcmp0(parsedMessage->params[0], "user:password") == 0);
	TEST_ASSERT(strcmp(parsedMessage->command, "PASS") == 0);
	TEST_ASSERT(parsedMessage->trailing == NULL);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
}
