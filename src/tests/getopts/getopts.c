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
#include "util.h"
#include "modules/getopts/getopts.h"

#include "api.h"

MODULE_NAME("test_getopts");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the getopts module");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("getopts", 0, 1, 1));

TEST_CASE(getopts);

TEST_SUITE_BEGIN(getopts)
	TEST_CASE_ADD(getopts);
TEST_SUITE_END

TEST_CASE(getopts)
{
	char *argv[] = {
		"-a",           // Short option without argument
		"-b", "100",    // Short option with argument
		"--c",          // Long option without argument
		"---", "100",   // Tokens that don't belong anywhere
		"--d=",         // Long option with empty argument
		"--e=foo",      // Long option with argument
		"--world=hallo",
		"-w welt",
		"--",           // End of token list, everything hereafter should be dismissed
		"--b=200",
		"-e",
		"bar",
		"-f",
		"--g",
		NULL
	};

	$$(void, setArgv)(argv);
	$$(void, setArgc)(sizeof(argv) / sizeof(argv[0]));
	$(void, getopts, setOptsParsed)(false);

	char *opt;
	// Short option without argument, should have an empty value (!= NULL)
	TEST_ASSERT((opt = $(char *, getopts, getOpt)("a")) != NULL);
	TEST_ASSERT(*opt == '\0');

	// Short option with argument
	TEST_ASSERT((opt = $(char *, getopts, getOpt)("b")) != NULL);
	TEST_ASSERT(strcmp(opt, "100") == 0);

	// Long option without argument, again expecting an empty value
	TEST_ASSERT((opt = $(char *, getopts, getOpt)("c")) != NULL);
	TEST_ASSERT(*opt == '\0');

	// Long option with empty argument
	TEST_ASSERT((opt = $(char *, getopts, getOpt)("d")) != NULL);
	TEST_ASSERT(*opt == '\0');

	// Long option with argument
	TEST_ASSERT((opt = $(char *, getopts, getOpt)("e")) != NULL);
	TEST_ASSERT(strcmp(opt, "foo") == 0);

	// A few options that should NOT exist
	TEST_ASSERT($(char *, getopts, getOpt)("f") == NULL);
	TEST_ASSERT($(char *, getopts, getOpt)("g") == NULL);
	TEST_ASSERT($(char *, getopts, getOpt)("-") == NULL);
	TEST_ASSERT($(char *, getopts, getOpt)("==") == NULL);

	// Test getOptValue
	TEST_ASSERT((opt = $(char *, getopts, getOptValue)("world", "w", NULL)) != NULL);
	TEST_ASSERT(strcmp(opt, "hallo") == 0);

	TEST_ASSERT((opt = $(char *, getopts, getOptValue)("w", "world", NULL)) != NULL);
	TEST_ASSERT(strcmp(opt, "welt"));

	TEST_ASSERT((opt = $(char *, getopts, getOptValue)(NULL, NULL)) == NULL);

	TEST_PASS;
}

