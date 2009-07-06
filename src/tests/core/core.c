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
#include "hooks.h"
#include "module.h"

#include "api.h"

TEST_CASE(version_compare);
TEST_CASE(hooks);
TEST_CASE(module_failure);
HOOK_LISTENER(add_count);
HOOK_LISTENER(add_count_times);

static int counter = 0;

TEST_SUITE_BEGIN(core)
	TEST_CASE_ADD(version_compare);
	TEST_CASE_ADD(hooks);
	TEST_CASE_ADD(module_failure);
TEST_SUITE_END

TEST_CASE(version_compare)
{
	Version *a = createVersion(1, 2, 3, 4);

	TEST_ASSERT(compareVersions(a, a) == 0);

	Version *b = createVersion(1, 2, 3, 5);

	TEST_ASSERT(compareVersions(a, b) < 0);

	b->minor = 1;

	TEST_ASSERT(compareVersions(a, b) > 0);

	TEST_PASS;
}

TEST_CASE(hooks)
{
	HOOK_ADD(test);
	HOOK_ATTACH(test, add_count);

	HOOK_TRIGGER(test, 1);
	TEST_ASSERT(counter == 1);

	int factor = 2;
	HOOK_ATTACH_EX(test, add_count_times, &factor);

	HOOK_TRIGGER(test, 2);
	TEST_ASSERT(counter == 7);

	HOOK_DETACH_EX(test, add_count_times, &factor);

	HOOK_TRIGGER(test, 3);
	TEST_ASSERT(counter == 10);

	HOOK_DEL(test);
	TEST_PASS;
}

TEST_CASE(module_failure)
{
	// Request and revoke non existant module
	TEST_ASSERT(!requestModule("_doesnotexist_"));
	TEST_ASSERT(!revokeModule("_doesnotexist_"));

	TEST_PASS;
}

HOOK_LISTENER(add_count)
{
	int count = HOOK_ARG(int);

	counter += count;
}

HOOK_LISTENER(add_count_times)
{
	int count = HOOK_ARG(int);
	int factor = *(int *) custom_data;

	counter += count * factor;
}

API GList *module_depends()
{
	return NULL;
}
