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

#ifndef LANG_LUA_LANG_LUA_H
#define LANG_LUA_LANG_LUA_H

#include <glib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "modules/store/store.h"


/**
 * Evaluates a lua command
 *
 * @param command		Lua code to evaluate
 * @result				true if successful
 */
API bool evaluateLua(char *command);

/**
 * Evaluates a lua script
 *
 * @param filename		filename of the Lua script to evaluate
 * @result				true if successful
 */
API bool evaluateLuaScript(char *filename);

/**
 * Pops the last returned string from Lua's stack
 *
 * @result		the last string on the stack, must be freed by the caller. returns NULL if top stack element is no string
 */
API char *popLuaString();

/**
 * Pops the last returned store from Lua's stack
 *
 * @result		the last store on the stack, must be freed by the caller, returns NULL if top stack element is no store
 */
API Store *popLuaStore();

/**
 * Returns the currently active global Lua state
 *
 * @result		the currently active global Lua state - changes will effect all scripts in this context immediately
 */
API lua_State *getGlobalLuaState();

#endif
