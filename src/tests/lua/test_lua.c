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
#include "modules/lua/module_lua.h"

#define API

MODULE_NAME("test_lua");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the lua module");
MODULE_VERSION(0, 4, 3);
MODULE_BCVERSION(0, 4, 3);
MODULE_DEPENDS(MODULE_DEPENDENCY("lua", 0, 8, 0), MODULE_DEPENDENCY("xcall", 0, 2, 2), MODULE_DEPENDENCY("store", 0, 6, 3));

TEST(lua2store);
TEST(store2lua);
TEST(store2lua_rootlist);
TEST(store2lua_fail);
TEST(xcall_invoke);
TEST(xcall_define);
TEST(xcall_define_error);
TEST(xcall_direct_call);

TEST_SUITE_BEGIN(lua)
	ADD_SIMPLE_TEST(lua2store);
	ADD_SIMPLE_TEST(store2lua);
	ADD_SIMPLE_TEST(store2lua_rootlist);
	ADD_SIMPLE_TEST(store2lua_fail);
	ADD_SIMPLE_TEST(xcall_invoke);
	ADD_SIMPLE_TEST(xcall_define);
	ADD_SIMPLE_TEST(xcall_define_error);
	ADD_SIMPLE_TEST(xcall_direct_call);
TEST_SUITE_END

TEST(lua2store)
{
	char *check;
	TEST_ASSERT($(bool, lua, evaluateLua)("store = parseStore('bird = word; array = { key = value; list = (1 1 2 3 5 7 13 21) }')"));

	TEST_ASSERT($(bool, lua, evaluateLua)("return store.bird"));
	check = $(char *, lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "word") == 0);
	free(check);

	TEST_ASSERT($(bool, lua, evaluateLua)("return type(store.array)"));
	check = $(char *, lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "table") == 0);
	free(check);

	TEST_ASSERT($(bool, lua, evaluateLua)("return type(store.array.list)"));
	check = $(char *, lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "table") == 0);
	free(check);

	TEST_ASSERT($(bool, lua, evaluateLua)("return # store.array.list"));
	check = $(char *, lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "8") == 0);
	free(check);

	TEST_ASSERT($(bool, lua, evaluateLua)("return store.array.list[7]"));
	check = $(char *, lua, popLuaString)();
	TEST_ASSERT(g_strcmp0(check, "13") == 0);
	free(check);
}

TEST(store2lua)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("return {int = 17, float = 3.14, string = 'hello world', array = {foo = 'bar'}, list = {1, 1, 2, 3, 5, 8}, nolist = {4, 2, answer = 42}}"));

	Store *parsed;

	TEST_ASSERT((parsed = $(Store *, lua, popLuaStore)()) != NULL);

	Store *ret;
	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "int")) != NULL);
	TEST_ASSERT(ret->type == STORE_INTEGER);
	TEST_ASSERT(ret->content.integer == 17);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "float")) != NULL);
	TEST_ASSERT(ret->type == STORE_FLOAT_NUMBER);
	TEST_ASSERT(ret->content.float_number == 3.14f);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "string")) != NULL);
	TEST_ASSERT(ret->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(ret->content.string, "hello world") == 0);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "array/foo")) != NULL);
	TEST_ASSERT(ret->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(ret->content.string, "bar") == 0);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "list")) != NULL);
	TEST_ASSERT(ret->type == STORE_LIST);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "list/2")) != NULL);
	TEST_ASSERT(ret->type == STORE_INTEGER);
	TEST_ASSERT(ret->content.integer == 2);

	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "nolist/answer")) != NULL);
	TEST_ASSERT(ret->type == STORE_INTEGER);
	TEST_ASSERT(ret->content.integer == 42);

	$(void, store, freeStore)(parsed);
}

TEST(store2lua_rootlist)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("return {{}}"));

	Store *parsed;

	TEST_ASSERT((parsed = $(Store *, lua, popLuaStore)()) != NULL);
	TEST_ASSERT(parsed->type == STORE_ARRAY);

	Store *ret;
	TEST_ASSERT((ret = $(Store *, store, getStorePath)(parsed, "1")) != NULL);
	TEST_ASSERT(ret->type == STORE_LIST);
	TEST_ASSERT(g_queue_get_length(ret->content.list) == 0);

	$(void, store, freeStore)(parsed);
}

TEST(store2lua_fail)
{
	// Test non-string values and error cascading
	TEST_ASSERT($(bool, lua, evaluateLua)("function foo() return 42; end; return {{foo}}"));
	TEST_ASSERT($(Store *, lua, popLuaStore)() == NULL);

	// Test non-table argument
	TEST_ASSERT($(bool, lua, evaluateLua)("return 42"));
	TEST_ASSERT($(Store *, lua, popLuaStore)() == NULL);
}

TEST(xcall_invoke)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("return invokeXCall('xcall = { function = some_non_existing_function }');"));

	char *ret = $(char *, lua, popLuaString)();

	Store *retstore = $(Store *, store, parseStoreString)(ret);

	TEST_ASSERT(retstore != NULL);

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(retstore, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(retstore);
	free(ret);
}

TEST(xcall_define)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("function f(x) return 'bird = word' end"));
	TEST_ASSERT($(bool, lua, evaluateLua)("addXCallFunction('luatest', f)"));

	Store *retstore = $(Store *, xcall, invokeXCallByString)("xcall = { function = luatest }");

	Store *bird;
	TEST_ASSERT((bird = $(Store *, store, getStorePath)(retstore, "bird")) != NULL);
	TEST_ASSERT(bird->type == STORE_STRING);
	TEST_ASSERT(g_strcmp0(bird->content.string, "word") == 0);

	$(void, store, freeStore)(retstore);

	TEST_ASSERT($(bool, lua, evaluateLua)("delXCallFunction('luatest')"));
}

TEST(xcall_define_error)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("function g() return 42 end"));
	TEST_ASSERT($(bool, lua, evaluateLua)("function f(x) return g end"));
	TEST_ASSERT($(bool, lua, evaluateLua)("addXCallFunction('luatest', f)"));

	Store *retstore = $(Store *, xcall, invokeXCallByString)("xcall = { function = luatest }");

	Store *error;
	TEST_ASSERT((error = $(Store *, store, getStorePath)(retstore, "xcall/error")) != NULL);
	TEST_ASSERT(error->type == STORE_STRING);

	$(void, store, freeStore)(retstore);

	TEST_ASSERT($(bool, lua, evaluateLua)("delXCallFunction('luatest')"));
}

TEST(xcall_direct_call)
{
	TEST_ASSERT($(bool, lua, evaluateLua)("function f() return 'ret=42' end"));
	TEST_ASSERT($(bool, lua, evaluateLua)("addXCallFunction('luatest', f)"));
	TEST_ASSERT($(bool, lua, evaluateLua)("return luatest().ret"));

	char *ret;
	TEST_ASSERT((ret = $(char *, lua, popLuaString)()) != NULL);
	TEST_ASSERT(g_strcmp0(ret, "42") == 0);
	free(ret);

	TEST_ASSERT($(bool, lua, evaluateLua)("delXCallFunction('luatest')"));
}
