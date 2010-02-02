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
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"
#include "modules/lang_lua/lang_lua.h"

#include "api.h"

MODULE_NAME("test_lang_lua");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the lang_lua module");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("lang_lua", 0, 3, 1), MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 5, 3));

TEST_CASE(store);
TEST_CASE(xcall_invoke);
TEST_CASE(xcall_define);
TEST_CASE(xcall_define_error);

TEST_SUITE_BEGIN(lang_lua)
	TEST_CASE_ADD(store);
	TEST_CASE_ADD(xcall_invoke);
	TEST_CASE_ADD(xcall_define);
	TEST_CASE_ADD(xcall_define_error);
TEST_SUITE_END

TEST_CASE(store)
{
	char *check;
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("store = parseStore('bird = word; array = { key = value; list = (1 1 2 3 5 7 13 21) }')"));

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return store.bird"));
	check = $(char *, lang_lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "word") == 0);
	free(check);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return type(store.array)"));
	check = $(char *, lang_lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "table") == 0);
	free(check);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return type(store.array.list)"));
	check = $(char *, lang_lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "table") == 0);
	free(check);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return # store.array.list"));
	check = $(char *, lang_lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "8") == 0);
	free(check);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return store.array.list[7]"));
	check = $(char *, lang_lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "13") == 0);
	free(check);

	TEST_PASS;
}

TEST_CASE(xcall_invoke)
{
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("return invokeXCall('xcall = { function = some_non_existing_function }');"));

	char *ret = $(char *, lang_lua, popLuaString)();

	Store *retstore = $(Store *, store, parseStoreString)(ret);

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(retstore, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(retstore);
	free(ret);

	TEST_PASS;
}

TEST_CASE(xcall_define)
{
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("function f(x) return 'bird = word' end"));
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("addXCallFunction('luatest', f)"));

	GString *ret = $(GString *, xcall, invokeXCall)("xcall = { function = luatest }");

	Store *retstore = $(Store *, store, parseStoreString)(ret->str);

	Store *bird;
	TEST_ASSERT((bird = $(Store *, store, getStorePath)(retstore, "bird")) != NULL);
	TEST_ASSERT(bird->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(bird->content.string, "word") == 0);

	$(void, store, freeStore)(retstore);
	g_string_free(ret, true);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("delXCallFunction('luatest')"));

	TEST_PASS;
}

TEST_CASE(xcall_define_error)
{
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("function g() return 42 end"));
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("function f(x) return g end"));
	TEST_ASSERT($(bool, lang_lua, evaluateLua)("addXCallFunction('luatest', f)"));

	GString *ret = $(GString *, xcall, invokeXCall)("xcall = { function = luatest }");

	Store *retstore = $(Store *, store, parseStoreString)(ret->str);

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(retstore, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(retstore);
	g_string_free(ret, true);

	TEST_ASSERT($(bool, lang_lua, evaluateLua)("delXCallFunction('luatest')"));

	TEST_PASS;
}
