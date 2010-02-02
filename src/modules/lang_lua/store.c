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
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "api.h"
#include "lang_lua.h"
#include "store.h"

static int lua_parseStore(lua_State *state);
static void parseLuaStore(lua_State *state, Store *store);
static void parseLuaStoreArrayNode(void *key_p, void *value_p, void *state_p);

/**
 * Registers the lua store C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the store functions with
 * @result			true if successful
 */
API void luaInitStateStore(lua_State *state)
{
	lua_pushcfunction(state, &lua_parseStore);
	lua_setglobal(state, "parseStore");
}

/**
 * Lua C function to parse a store string into a Lua table
 *
 * @param state		the lua interpreter state during execution of the C function
 * @result			the number of parameters on the lua stack
 */
static int lua_parseStore(lua_State *state)
{
	const char *storestr = luaL_checkstring(state, 1);

	Store *store = $(Store *, store, parseStoreString)(storestr);

	if(store == NULL) {
		lua_newtable(state);
	} else {
		parseLuaStore(state, store);
	}

	return 1;
}

/**
 * Parses a store into a Lua table
 *
 * @param state		the state in which the store should be parsed into a table
 * @param store		the store to parse
 */
static void parseLuaStore(lua_State *state, Store *store)
{
	switch(store->type) {
		case STORE_ARRAY:
			lua_newtable(state);
			g_hash_table_foreach(store->content.array, &parseLuaStoreArrayNode, state);
		break;
		case STORE_LIST:
			lua_newtable(state);
			int i = 0;
			for(GList *iter = store->content.list->head; iter != NULL; iter = iter->next) {
				lua_pushinteger(state, i++); // create key for Lua table entry
				parseLuaStore(state, iter->data); // create value for Lua table entry
				lua_settable(state, -3); // add table entry
			}
		break;
		case STORE_STRING:
			lua_pushstring(state, store->content.string);
		break;
		case STORE_INTEGER:
			lua_pushinteger(state, store->content.integer);
		break;
		case STORE_FLOAT_NUMBER:
			lua_pushnumber(state, store->content.float_number);
		break;
	}
}

/**
 * A GHFunc to parse a store array into a Lua table
 *
 * @param key_p			a pointer to the key
 * @param value_p		a pointer to the value
 * @param state_p		a pointer to the lua state to parse into
 */
static void parseLuaStoreArrayNode(void *key_p, void *value_p, void *state_p)
{
	char *key = key_p;
	Store *value = value_p;
	lua_State *state = state_p;
	lua_pushstring(state, key); // create key for Lua table entry
	parseLuaStore(state, value); // create value for Lua table entry
	lua_settable(state, -3); // add table entry
}
