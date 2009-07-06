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
#include <stdio.h>
#include "dll.h"
#include "test.h"
#include "modules/std_config/std_config.h"
#include "modules/config/config.h"
#include "modules/config/path.h"

#include "api.h"

#define PARENT_INT_VALUE_PATH "default"
#define INT_VALUE_PATH "default/int"

TEST_CASE(simpleUserOverwriteConfig);

TEST_SUITE_BEGIN(std_config)
	TEST_CASE_ADD(simpleUserOverwriteConfig);
TEST_SUITE_END

TEST_CASE(simpleUserOverwriteConfig)
{
	Config *userConfig = getStandardConfig(USER_OVERWRITE_CONFIG);
	TEST_ASSERT(userConfig != NULL);

	setConfigPath(userConfig, PARENT_INT_VALUE_PATH, createConfigNodes());
	setConfigPath(userConfig, INT_VALUE_PATH, createConfigIntegerValue(500));

	ConfigNodeValue *value = getConfigPathSubtree(userConfig, INT_VALUE_PATH);
	TEST_ASSERT(value->type == CONFIG_INTEGER);
	TEST_ASSERT(*((int *)getConfigValueContent(value)) == 500);

	saveStandardConfig(USER_OVERWRITE_CONFIG);

	TEST_PASS;
}

API GList *module_depends()
{
	return g_list_append(NULL, "std_config");
}
