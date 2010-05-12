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

#include "api.h"

MODULE_NAME("test_irc_parser");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the irc_parser module");
MODULE_VERSION(0, 0, 2);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_parser", 0, 1, 0));

TEST_CASE(utf8Trailing);
TEST_CASE(whitespaces);
TEST_CASE(userMask);
TEST_CASE(ping);
TEST_CASE(noticeAuth);
TEST_CASE(serverNotice);
TEST_CASE(justQuitMessage);

TEST_SUITE_BEGIN(irc_parser)
	TEST_CASE_ADD(utf8Trailing);
	TEST_CASE_ADD(whitespaces);
	TEST_CASE_ADD(userMask);
	TEST_CASE_ADD(ping);
	TEST_CASE_ADD(noticeAuth);
	TEST_CASE_ADD(serverNotice);
	TEST_CASE_ADD(justQuitMessage);
TEST_SUITE_END

TEST_CASE(utf8Trailing)
{
	char *message = g_strdup("Someone :Зарегистрируйтесь Unicode แผ่นดินฮั่นเสื่อมโทรมแสนสังเวช 1234567890 ╔══╦══╗  ┌──┬──┐  ╭──┬──╮  ╭──┬──╮\r\n");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);

	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(strcmp(parsedMessage->command, "Someone") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "Зарегистрируйтесь Unicode แผ่นดินฮั่นเสื่อมโทรมแสนสังเวช 1234567890 ╔══╦══╗  ┌──┬──┐  ╭──┬──╮  ╭──┬──╮") == 0);

	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(parsedMessage->params_count == 0);
	TEST_ASSERT(parsedMessage->prefix == NULL);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	free(message);

	TEST_PASS;
}

TEST_CASE(whitespaces)
{
	char *message = g_strdup(":irc.gamesurge.net            366           Gregor          @         #php.de         :    Do         something!\r\n");

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
	free(message);

	TEST_PASS;
}

TEST_CASE(userMask)
{
	char *message = g_strdup(":Gregor!kalisko@kalisko.org KICK #php.de Someone :blub");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);

	TEST_ASSERT(parsedMessage);

	IrcUserMask *userMask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(parsedMessage->prefix);

	TEST_ASSERT(strcmp(userMask->nick, "Gregor") == 0);
	TEST_ASSERT(strcmp(userMask->user, "kalisko") == 0);
	TEST_ASSERT(strcmp(userMask->host, "kalisko.org") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	$(void, irc_parser, freeIrcUserMask)(userMask);
	free(message);

	TEST_PASS;
}

TEST_CASE(ping)
{
	char *message = g_strdup("PING :irc.gamesurge.net");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(strcmp(parsedMessage->command, "PING") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "irc.gamesurge.net") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	free(message);

	TEST_PASS;
}

TEST_CASE(noticeAuth)
{
	char *message = g_strdup("NOTICE AUTH :*** Looking up your hostname");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(strcmp(parsedMessage->params[0], "AUTH") == 0);
	TEST_ASSERT(parsedMessage->params[1] == NULL);
	TEST_ASSERT(parsedMessage->params_count == 1);
	TEST_ASSERT(strcmp(parsedMessage->command, "NOTICE") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "*** Looking up your hostname") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	free(message);

	TEST_PASS;
}

TEST_CASE(serverNotice)
{
	char *message = g_strdup(":Staff.CA.US.GameSurge.net NOTICE * :*** Notice -- Received KILL message for grog. From Someone Path: Someone.operator.support!Someone (.)");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(strcmp(parsedMessage->prefix, "Staff.CA.US.GameSurge.net") == 0);
	TEST_ASSERT(strcmp(parsedMessage->params[0], "*") == 0);
	TEST_ASSERT(parsedMessage->params[1] == NULL);
	TEST_ASSERT(parsedMessage->params_count == 1);
	TEST_ASSERT(strcmp(parsedMessage->command, "NOTICE") == 0);
	TEST_ASSERT(strcmp(parsedMessage->trailing, "*** Notice -- Received KILL message for grog. From Someone Path: Someone.operator.support!Someone (.)") == 0);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	free(message);

	TEST_PASS;
}

/**
 * Testcase for bug report #1406:
 * The irc_parser module currently fails to parse messages that have nothing but a command statement in them.
 * Practical examples include the commands "AWAY" and "QUIT" which may be sent without trailing content by clients.
 */
TEST_CASE(justQuitMessage)
{
	char *message = g_strdup("AWAY");

	IrcMessage *parsedMessage = $(IrcMessage *, irc_parser, parseIrcMessage)(message);
	TEST_ASSERT(parsedMessage);

	TEST_ASSERT(parsedMessage->prefix == NULL);
	TEST_ASSERT(parsedMessage->params == NULL);
	TEST_ASSERT(parsedMessage->params_count == 0);
	TEST_ASSERT(strcmp(parsedMessage->command, "AWAY") == 0);
	TEST_ASSERT(parsedMessage->trailing == NULL);

	$(void, irc_parser, freeIrcMessage)(parsedMessage);
	free(message);

	TEST_PASS;
}
