/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "modules/config/config.h"
#include "modules/store/parse.h"
#include "modules/store/store.h"


#define API

MODULE_NAME("test_config");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the config module");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("store", 0, 5, 3));

TEST_CASE(simple_readonly);
TEST_CASE(writable_change_save);

TEST_SUITE_BEGIN(config)
	TEST_CASE_ADD(simple_readonly);
	TEST_CASE_ADD(writable_change_save);
TEST_SUITE_END

TEST_CASE(simple_readonly)
{
	Store *testConfig = $(Store *, store, parseStoreString)(
		"profileA = {"
		"	keyA1 = valueA1"
		"	keyA2 = valueA2"
		"}"
		""
		"profileB = {"
		"	keyB1 = valueB1"
		"	keyB2 = valueB2"
		"}"
	);

	Store *oldReadOnly = $(Store *, config, injectReadOnlyConfig)(testConfig);

	TEST_ASSERT(strcmp($(Store *, config, getConfigPath)("profileA/keyA1")->content.string, "valueA1") == 0);
	TEST_ASSERT(strcmp($(Store *, config, getConfigPath)("profileA/keyA2")->content.string, "valueA2") == 0);

	// clean up
	$(Store *, config, injectReadOnlyConfig)(oldReadOnly);

	$(void, store, freeStore)(testConfig);

}

TEST_CASE(writable_change_save)
{
	Store *changedWritableConfig = $(Store *, store, parseStoreString)(
		"profileA = {"
		"	keyA1 = valueA1"
		"	keyA2 = valueA2"
		"}"
	);

	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA1") == NULL);
	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA2") == NULL);

#ifdef WIN32
	char *oldPath = $(char *, config, injectWritableConfigFilePath)("NUL");
#else
	char *oldPath = $(char *, config, injectWritableConfigFilePath)("/dev/null/");
#endif
	Store *oldWritable = $(Store *, config, injectWritableConfig)(changedWritableConfig, false);

	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA1") == NULL);
	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA2") == NULL);

	$(void, config, saveWritableConfig)();

	TEST_ASSERT(strcmp($(Store *, config, getConfigPath)("profileA/keyA1")->content.string, "valueA1") == 0);
	TEST_ASSERT(strcmp($(Store *, config, getConfigPath)("profileA/keyA2")->content.string, "valueA2") == 0);

	// clean up
	$(char *, config, injectWritableConfigFilePath)(oldPath);
	$(char *, config, injectWritableConfig)(oldWritable, true);

	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA1") == NULL);
	TEST_ASSERT($(Store *, config, getConfigPath)("profileA/keyA2") == NULL);

}
