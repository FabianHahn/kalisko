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
#include "modules/string_util/string_util.h"

#define API

MODULE_NAME("test_string_util");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the string_util module");
MODULE_VERSION(0, 0, 2);
MODULE_BCVERSION(0, 0, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("string_util", 0, 1, 0));

TEST(stripspaces);

TEST_SUITE_BEGIN(string_util)
	ADD_SIMPLE_TEST(stripspaces);
TEST_SUITE_END

TEST(stripspaces)
{
	char *message = g_strdup("      a\r \n       \t \fb\r       \v           ");

	$(void, string_util, stripDuplicateWhitespace)(message);
	TEST_ASSERT(strlen(message) == 5);

	free(message);
}

