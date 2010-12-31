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
#include "modules/string_format/string_format.h"

#include "api.h"

MODULE_NAME("test_string_format");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the string_format module");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("string_format", 0, 0, 1));

TEST_CASE(empty);
TEST_CASE(no_replacements);
TEST_CASE(formatting);

TEST_SUITE_BEGIN(string_format)
	TEST_CASE_ADD(empty);
	TEST_CASE_ADD(no_replacements);
	TEST_CASE_ADD(formatting);
TEST_SUITE_END

#define string_format $(char*, string_format, format_string)

TEST_CASE(empty)
{
	char *formatted = string_format("", NULL);
	TEST_ASSERT(*formatted == 0);
	TEST_PASS;
}

TEST_CASE(no_replacements)
{
	char *str = "The quick brown fox jumps over the lazy dog.";
	char *formatted = string_format(str, NULL);
	TEST_ASSERT(strcmp(str, formatted) == 0);
	TEST_PASS;
}

TEST_CASE(formatting)
{
	char *str = "before {eins} {zwei} between {with space} {eins} {drei} end";
	char *formatted = string_format(str, "eins", "zwei", "zwei", "drei", NULL);
	TEST_ASSERT(strcasecmp(formatted, "before zwei drei between  zwei  end") == 0);
	free(formatted);

	TEST_PASS;
}

