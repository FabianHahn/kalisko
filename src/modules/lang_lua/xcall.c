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

#include <assert.h>
#include <glib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "modules/xcall/xcall.h"
#include "api.h"
#include "lang_lua.h"
#include "xcall.h"
#include "store.h"

static bool unregisterLuaXCallFunctions(void *key_p, void *value_p, void *data_p);
static bool unregisterLuaXCallFunction(void *key_p, void *value_p, void *data_p);
static int lua_invokeXCall(lua_State *state);
static int lua_addXCallFunction(lua_State *state);
static int lua_delXCallFunction(lua_State *state);
static Store *xcall_luaXCallFunction(Store *xcall);
static Store *xcall_evaluateLua(Store *xcall);
static Store *xcall_evaluateLuaScript(Store *xcall);

static GHashTable *stateFunctions;
static GHashTable *functionState;

/**
 * Initializes the Lua XCall interface
 */
API void luaInitXCall()
{
	// Register global xcall functions
	$(bool, xcall, addXCallFunction)("evaluateLua", &xcall_evaluateLua);
	$(bool, xcall, addXCallFunction)("evaluateLuaScript", &xcall_evaluateLuaScript);

	stateFunctions = g_hash_table_new(&g_direct_hash, &g_direct_equal);
	functionState = g_hash_table_new(&g_str_hash, &g_str_equal);
}

/**
 * Frees the Lua XCall interface
 */
API void luaFreeXCall()
{
	// Unregister global xcall functions
	$(bool, xcall, delXCallFunction)("evaluateLua");
	$(bool, xcall, delXCallFunction)("evaluateLuaScript");

	g_hash_table_foreach_remove(stateFunctions, &unregisterLuaXCallFunctions, NULL);
	g_hash_table_destroy(stateFunctions);
	g_hash_table_destroy(functionState);
}

/**
 * Registers the lua xcall C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the xcall functions with
 * @result			true if successful
 */
API bool luaInitStateXCall(lua_State *state)
{
	GHashTable *functionRefs;

	if(g_hash_table_lookup(stateFunctions, state) != NULL) {
		LOG_ERROR("Cannot init Lua XCall interface: State %p already has an XCall functionRefs entry", state);
		return false;
	}

	functionRefs = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &free);
	g_hash_table_insert(stateFunctions, state, functionRefs);

	lua_pushcfunction(state, &lua_invokeXCall);
	lua_setglobal(state, "invokeXCall");
	lua_pushcfunction(state, &lua_addXCallFunction);
	lua_setglobal(state, "addXCallFunction");
	lua_pushcfunction(state, &lua_delXCallFunction);
	lua_setglobal(state, "delXCallFunction");

	return true;
}

/**
 * Unregisters the lua xcall C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the xcall functions with
 * @result			true if successful
 */
API bool luaFreeStateXCall(lua_State *state)
{
	GHashTable *functionRefs;
	if((functionRefs = g_hash_table_lookup(stateFunctions, state)) == NULL) {
		LOG_ERROR("Cannot free Lua XCall interface: State %p doesn't have an XCall functionRefs entry", state);
		return false;
	}

	g_hash_table_foreach_remove(functionRefs, &unregisterLuaXCallFunction, state); // Remove all functions in the functionRefs table

	return true;
}

/**
 * A GHRFunc that unregisters all lua functions of a state
 *
 * @param key_p			a pointer to the state to consider
 * @param value_p		a pointer to the functionRefs GHashTable to consider
 * @param data_p		unused
 */
static bool unregisterLuaXCallFunctions(void *key_p, void *value_p, void *data_p)
{
	lua_State *state = key_p;
	GHashTable *functionRefs = value_p;

	g_hash_table_foreach_remove(functionRefs, &unregisterLuaXCallFunction, state); // Remove all functions in the functionRefs table

	return true; // remove entry
}

/**
 * A GHRFunc that unregisters a lua function
 *
 * @param key_p			a pointer to the name of the xcall function
 * @param value_p		a pointer to the lua ref of the lua function
 * @param data_p		the lua state to consider
 */
static bool unregisterLuaXCallFunction(void *key_p, void *value_p, void *data_p)
{
	char *name = key_p;
	int *ref = value_p;
	lua_State *state = data_p;

	$(bool, xcall, delXCallFunction)(name); // delete the actual xcall
	luaL_unref(state, LUA_REGISTRYINDEX, *ref); // unref the stored lua function

	g_hash_table_remove(functionState, name); // also remove the functionState entry as long as the key is not freed yet ;)

	return true; // remove entry
}

/**
 * Lua C function to invoke an XCall
 *
 * @param state		the lua interpreter state during execution of the C function
 * @result			the number of parameters on the lua stack
 */
static int lua_invokeXCall(lua_State *state)
{
	const char *xcall = luaL_checkstring(state, 1);

	Store *ret = $(Store *, xcall, invokeXCallByString)(xcall);
	GString *retstr = $(GString *, store, writeStoreGString)(ret);
	lua_pushstring(state, retstr->str);
	g_string_free(retstr, true);
	$(void, store, freeStore)(ret);

	return 1;
}

/**
 * Lua C function to add an XCall function
 *
 * @param state		the lua interpreter state during execution of the C function
 * @result			the number of parameters on the lua stack
 */
static int lua_addXCallFunction(lua_State *state)
{
	const char *name = luaL_checkstring(state, 1);
	luaL_checktype(state, 2, LUA_TFUNCTION);

	int ref = luaL_ref(state, LUA_REGISTRYINDEX); // pop function from stack and store in registry
	lua_rawgeti(state, LUA_REGISTRYINDEX, ref); // push function back from stack to ensure consistency

	GHashTable *functionRefs;
	if((functionRefs = g_hash_table_lookup(stateFunctions, state)) == NULL) {
		LOG_ERROR("lua_addXCallFunction: Cannot find functionRefs for state %p", state);
		lua_pushboolean(state, false);
		return 1;
	}

	if(!$(bool, xcall, addXCallFunction)(name, &xcall_luaXCallFunction)) { // Failed to add XCall
		LOG_ERROR("lua_addXCallFunction: Failed to add XCall function '%s'", name);
		lua_pushboolean(state, false);
		return 1;
	}

	int *ref_container = ALLOCATE_OBJECT(int);
	*ref_container = ref;

	char *dupname = strdup(name);
	g_hash_table_insert(functionRefs, dupname, ref_container);
	g_hash_table_insert(functionState, dupname, state);

	LOG_INFO("Added Lua XCall function '%s'", name);

	lua_pushboolean(state, true);
	return 1;
}

/**
 * Lua C function to removes an XCall function
 *
 * @param state		the lua interpreter state during execution of the C function
 * @result			the number of parameters on the lua stack
 */
static int lua_delXCallFunction(lua_State *state)
{
	const char *name = luaL_checkstring(state, 1);

	lua_State *fstate = g_hash_table_lookup(functionState, name);

	if(fstate == NULL) {
		LOG_ERROR("lua_delXCallFunction: Cannot find Lua state for Lua XCall function name '%s'", name);
		lua_pushboolean(state, false);
		return 1;
	}

	GHashTable *functionRefs = g_hash_table_lookup(stateFunctions, fstate);

	if(functionRefs == NULL) {
		LOG_ERROR("lua_delXCallFunction: Cannot find functionRefs table for Lua XCall function name '%s' in state %p", name, fstate);
		lua_pushboolean(state, false);
		return 1;
	}

	int *refp = g_hash_table_lookup(functionRefs, name);

	if(refp == NULL) {
		LOG_ERROR("lua_delXCallFunction: Cannot find Lua XCall function reference for Lua XCall function name '%s' in state %p", name, fstate);
		lua_pushboolean(state, false);
		return 1;
	}

	unregisterLuaXCallFunction((char *) name, refp, fstate); // nothing will happen to the name pointer, I promise :)
	g_hash_table_remove(functionRefs, name);

	LOG_INFO("Removed Lua XCall function '%s'", name);

	lua_pushboolean(state, true);
	return 1;
}

/**
 * A XCallFunction for lua XCall functions
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_luaXCallFunction(Store *xcall)
{
	Store *function = $(Store *, store, getStorePath)(xcall, "xcall/function"); // retrieve function name
	Store *retstore = $(Store *, store, createStore)();

	assert(function != NULL && function->type == STORE_STRING);

	char *funcname = function->content.string;

	lua_State *state = g_hash_table_lookup(functionState, funcname);
	assert(state != NULL);
	GHashTable *functionRefs = g_hash_table_lookup(stateFunctions, state);
	assert(functionRefs != NULL);
	int *refp = g_hash_table_lookup(functionRefs, funcname);
	assert(refp != NULL);

	lua_rawgeti(state, LUA_REGISTRYINDEX, *refp); // push lua xcall function to stack
	parseStoreToLua(state, xcall); // parse the store into lua format and push it onto stack

	if(lua_pcall(state, 1, 1, 0) != 0) {
		GString *err = g_string_new("");
		g_string_append_printf(err, "Error running Lua XCall function '%s': %s", funcname, lua_tostring(state, -1));
		LOG_ERROR("%s", err->str);
		$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, stote, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
		g_string_free(err, true);
		return retstore;
	}

	if(!lua_isstring(state, -1)) {
		GString *err = g_string_new("");
		g_string_append_printf(err, "Error running Lua XCall function '%s': Returned value is no string", funcname);
		LOG_ERROR("%s", err->str);
		$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
		g_string_free(err, true);
		return retstore;
	}

	const char *ret = lua_tostring(state, -1); // Fetch return value from stack
	retstore = $(Store *, store, parseStoreString)(ret);
	lua_pop(state, 1);

	return retstore;
}

/**
 * XCallFunction to evaluate Lua code
 * XCall parameters:
 *  * string eval			the lua code to evaluate
 * XCall result:
 * 	* int success			nonzero if successful
 *  * string lua_error		error message from lua if unsuccessful
 *  * string lua_ret		return value from lua if successful and available
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_evaluateLua(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *eval = $(Store *, store, getStorePath)(xcall, "eval");

	if(eval == NULL || eval->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'eval'"));
	} else {
		char *code = eval->content.string;
		bool evaluated = evaluateLua(code);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(evaluated));

		if(!evaluated) {
			char *ret = $(char *, lang_lua, popLuaString)();
			$(bool, store, setStorePath)(retstore, "lua_error", $(Store *, store, createStoreStringValue)(ret));
			free(ret);
		} else {
			char *ret = $(char *, lang_lua, popLuaString)();
			if(ret != NULL) {
				$(bool, store, setStorePath)(retstore, "lua_ret", $(Store *, store, createStoreStringValue)(ret));
				free(ret);
			}
		}
	}

	return retstore;
}

/**
 * XCallFunction to evaluate a Lua script
 * XCall parameters:
 *  * string file			the lua code to evaluate
 * XCall result:
 * 	* int success			nonzero if successful
 *  * string lua_error		error message from lua if unsuccessful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_evaluateLuaScript(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *file = $(Store *, store, getStorePath)(xcall, "file");

	if(file == NULL || file->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'file'"));
	} else {
		char *filename = file->content.string;
		bool evaluated = evaluateLuaScript(filename);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(evaluated));

		if(!evaluated) {
			char *ret = $(char *, lang_lua, popLuaString)();
			$(bool, store, setStorePath)(retstore, "lua_error", $(Store *, store, createStoreStringValue)(ret));
			free(ret);
		}
	}

	return retstore;
}

