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

static Store *parseLuaToStoreRec(lua_State *state, bool allow_list);
static int lua_dumpStore(lua_State *state);
static int lua_parseStore(lua_State *state);
static void parseLuaStoreArrayNode(void *key_p, void *value_p, void *state_p);

/**
 * Registers the lua store C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the store functions with
 * @result			true if successful
 */
API void luaInitStateStore(lua_State *state)
{
	lua_pushcfunction(state, &lua_dumpStore);
	lua_setglobal(state, "dumpStore");
	lua_pushcfunction(state, &lua_parseStore);
	lua_setglobal(state, "parseStore");
}

/**
 * Parses a store into a Lua table
 *
 * @param state		the state in which the store should be parsed into a table
 * @param store		the store to parse
 */
API void parseStoreToLua(lua_State *state, Store *store)
{
	switch(store->type) {
		case STORE_ARRAY:
			lua_newtable(state);
			g_hash_table_foreach(store->content.array, &parseLuaStoreArrayNode, state);
		break;
		case STORE_LIST:
			lua_newtable(state);
			int i = 1;
			for(GList *iter = store->content.list->head; iter != NULL; iter = iter->next) {
				lua_pushinteger(state, i++); // create key for Lua table entry
				parseStoreToLua(state, iter->data); // create value for Lua table entry
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
 * Parses a Lua table on top of the Lua stack into a store
 *
 * @param state		the state in which the Lua table should be parsed into a store
 * @result			the parsed store which is either a list or an array, or NULL on failure
 */
API Store *parseLuaToStore(lua_State *state)
{
	return parseLuaToStoreRec(state, false);
}

/**
 * Recursive helper function to parse a Lua table on top of the Lua stack into a store
 *
 * @param state		the state in which the Lua table should be parsed into a store
 * @param bool		true if the table is allowed to be parsed as a list
 * @result			the parsed store or NULL on failure
 */
static Store *parseLuaToStoreRec(lua_State *state, bool allow_list)
{
	if(!lua_istable(state, -1)) {
		LOG_ERROR("Tried to parse Lua value to store which is not a table, aborting");
		return NULL;
	}

	GQueue *queue;
	GHashTable *hashtable;

	if(allow_list) { // we are allowed to read lists
		queue = g_queue_new();
		hashtable = NULL;
	} else { // only read arrays
		queue = NULL;
		hashtable = $(GHashTable *, store, createStoreNodes)();
	}

	// Initialize iterator
	lua_pushnil(state);
	int i = 1;

	while(lua_next(state, -2) != 0) { // Push next key-value pair to stack
		if(hashtable == NULL) { // we assume we're still constructing a list
			int index = lua_tointeger(state, -2); // retrieve the integer key
			int fltindex = lua_tonumber(state, -2); // retrieve the float key

			if(lua_isnumber(state, -2) && index == fltindex && index == i++) { // this is a followup index in a list
				if(lua_isnumber(state, -1)) { // the value is a number
					int intval = lua_tointeger(state, -1);
					float fltval = lua_tonumber(state, -1);

					if(intval == fltval) { // assume integer
						g_queue_push_tail(queue, $(Store *, store, createStoreIntegerValue)(intval));
					} else { // float number
						g_queue_push_tail(queue, $(Store *, store, createStoreFloatNumberValue)(fltval));
					}
				} else if(lua_isstring(state, -1)) { // the value is a string
					lua_pushvalue(state, -1); // Push value to stack to prevent actual Lua type to change
					const char *str = lua_tostring(state, -1);
					g_queue_push_tail(queue, $(Store *, store, createStoreStringValue)(str));
					lua_pop(state, 1); // pop copied value from stack
				} else { // the value is either a list or an array
					Store *value = parseLuaToStoreRec(state, true);

					if(value == NULL) { // value conversion failed, free all we've done so far
						for(GList *iter = queue->head; iter != NULL; iter = iter->next) {
							$(void, store, freeStore)(iter->data);
						}
						g_queue_free(queue);
						return NULL;
					}

					g_queue_push_tail(queue, value);
				}
			} else { // this is not a list, it's an array!
				hashtable = $(GHashTable *, store, createStoreNodes)(); // create hash table
				i = 1;
				for(GList *iter = queue->head; iter != NULL; iter = iter->next, i++) {
					// Transform integer index into string
					GString *intstr = g_string_new("");
					g_string_append_printf(intstr, "%d", i);

					// Add value to store array's hash table
					g_hash_table_insert(hashtable, strdup(intstr->str), iter->data);

					g_string_free(intstr, true);
				}

				g_queue_free(queue);
			}
		}

		if(hashtable != NULL) { // it's not a list, it's an array
			if(!lua_isstring(state, -2)) { // check if key can be converted to string
				LOG_ERROR("Lua table key is not a string, aborting conversion to store");
				g_hash_table_destroy(hashtable);
				return NULL;
			} else {
				lua_pushvalue(state, -2); // Push key to stack to prevent actual Lua type to change
				char *key = strdup(lua_tostring(state, -1)); // copy key string
				lua_pop(state, 1); // pop copied key from stack

				if(lua_isnumber(state, -1)) { // the value is a number
					int intval = lua_tointeger(state, -1);
					float fltval = lua_tonumber(state, -1);

					if(intval == fltval) { // assume integer
						g_hash_table_insert(hashtable, key, $(Store *, store, createStoreIntegerValue)(intval));
					} else { // float number
						g_hash_table_insert(hashtable, key, $(Store *, store, createStoreFloatNumberValue)(fltval));
					}
				} else if(lua_isstring(state, -1)) { // the value is a string
					const char *str = lua_tostring(state, -1);
					g_hash_table_insert(hashtable, key, $(Store *, store, createStoreStringValue)(str));
				} else { // the value is either a list or an array
					Store *value = parseLuaToStoreRec(state, true);

					if(value == NULL) { // value conversion failed, free all we've done so far
						g_hash_table_destroy(hashtable);
						free(key);
						return NULL;
					}

					g_hash_table_insert(hashtable, key, value);
				}
			}
		}

		// Prepare for next iteration
		lua_pop(state, 1); // pop value from stack, keep key
	}

	if(hashtable == NULL) { // We constructed a list
		return $(Store *, store, createStoreListValue)(queue);
	} else { // We constructed an array
		return $(Store *, store, createStoreArrayValue)(hashtable);
	}
}

/**
 * Lua C function to dump a Lua store table into a Lua string
 *
 * @param state		the lua interpreter state during execution of the C function
 * @result			the number of parameters on the lua stack
 */
static int lua_dumpStore(lua_State *state)
{
	if(!lua_istable(state, 1)) { // The passed parameter is no table
		lua_pushnil(state);
		return 1;
	}

	lua_pushvalue(state, 1); // Push the table parameter on top of the stack
	Store *store = parseLuaToStore(state); // parse the store
	lua_pop(state, 1); // pop copied value from stack

	if(store == NULL) { // parse failed
		lua_pushnil(state);
		return 1;
	}

	GString *str = $(GString *, store, writeStoreGString)(store); // write store to string
	lua_pushstring(state, str->str); // push return value to stack
	g_string_free(str, true); // free the temp string
	$(void, store, freeStore)(store); // free the parsed store

	return 1;
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
		parseStoreToLua(state, store);
		$(void, store, freeStore)(store);
	}

	return 1;
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
	parseStoreToLua(state, value); // create value for Lua table entry
	lua_settable(state, -3); // add table entry
}
