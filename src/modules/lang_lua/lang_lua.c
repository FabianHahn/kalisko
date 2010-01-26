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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "modules/xcall/xcall.h"
#include "api.h"
#include "lang_lua.h"
#include "xcall.h"

MODULE_NAME("lang_lua");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module provides access to the Lua scripting language");
MODULE_VERSION(0, 2, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 5, 3));

static lua_State *state;

MODULE_INIT
{
	luaInitXCall();

	if((state = lua_open()) == NULL) {
		LOG_ERROR("Could not initialize the Lua interpreter");
		return false;
	}

	luaL_openlibs(state);
	luaInitStateXCall(state);

	return true;
}

MODULE_FINALIZE
{
	luaFreeStateXCall(state);
	lua_close(state);

	luaFreeXCall();
}

/**
 * Evaluates a lua command
 *
 * @param cpmmand		Lua code to evaluate
 * @result				true if successful
 */
API bool evaluateLua(char *command)
{
	return luaL_dostring(state, command) == 0;
}

/**
 * Pops the last returned string from Lua's stack
 *
 * @result		the last string on the stack, must be freed by the caller
 */
API char *popLuaString()
{
	char *string = strdup(lua_tostring(state, -1));
	lua_pop(state, 1);
	return string;
}
