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
#include "api.h"
#include "xcall.h"

static int lua_invokeXCall(lua_State *state);

/**
 * Registers the lua xcall C functions for an interpreter
 *
 * @param state		the lua interpreter state to register the xcall functions with
 */
API void luaRegisterXCall(lua_State *state)
{

}

static int lua_invokeXCall(lua_State *state)
{
	const char *path = luaL_checkstring(state, 1);
#if 0
	/* open directory */
	dir = opendir(path);
	if(dir == NULL) { /* error opening the directory? */
		lua_pushnil(L); /* return nil and ... */
		lua_pushstring(L, strerror(errno)); /* error message */
		return 2; /* number of results */
	}

	/* create result table */
	lua_newtable(L);
	i = 1;
	while((entry = readdir(dir)) != NULL) {
		lua_pushnumber(L, i++); /* push key */
		lua_pushstring(L, entry->d_name); /* push value */
		lua_settable(L, -3);
	}

	closedir(dir);
	return 1; /* table is already on top */
#endif
}
