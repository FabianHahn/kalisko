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


#ifndef LANG_LUA_STORE_H
#define LANG_LUA_STORE_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "modules/store/store.h"


/**
 * Registers the lua store C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the store functions with
 */
API void initLuaStateStore(lua_State *state);

/**
 * Frees the lua store C functions for an interpreter
 *
 * @param state		the lua interpreter state to free the store functions for
 */
API void freeLuaStateStore(lua_State *state);

/**
 * Parses a store into a Lua table
 *
 * @param state		the state in which the store should be parsed into a table
 * @param store		the store to parse
 */
API void parseStoreToLua(lua_State *state, Store *store);

/**
 * Parses a Lua table on top of the Lua stack into a store
 *
 * @param state		the state in which the Lua table should be parsed into a store
 * @result			the parsed store which is either a list or an array, or NULL on failure
 */
API Store *parseLuaToStore(lua_State *state);

#endif
