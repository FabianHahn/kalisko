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
#include "version.h"
#include "module.h"

#define API

TEST_CASE(version_compare);
TEST_CASE(module_failure);

MODULE_NAME("test_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the Kalisko core");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 1);
MODULE_NODEPS;

TEST_SUITE_BEGIN(core)
	TEST_CASE_ADD(version_compare);
	TEST_CASE_ADD(module_failure);
TEST_SUITE_END

TEST_CASE(version_compare)
{
	Version *a = $$(Version *, createVersion)(1, 2, 3, 4);

	TEST_ASSERT($$(int, compareVersions)(a, a) == 0);

	Version *b = $$(Version *, createVersion)(1, 2, 3, 5);

	TEST_ASSERT($$(int, compareVersions)(a, b) < 0);

	b->minor = 1;

	TEST_ASSERT($$(int, compareVersions)(a, b) > 0);

	$$(void, freeVersion)(a);
	$$(void, freeVersion)(b);

	TEST_PASS;
}

TEST_CASE(module_failure)
{
	// Request and revoke non existent module
	TEST_ASSERT(!$$(bool, requestModule)("_doesnotexist_"));
	TEST_ASSERT(!$$(bool, revokeModule)("_doesnotexist_"));

	TEST_PASS;
}
