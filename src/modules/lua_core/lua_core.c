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
#include "types.h"
#include "log.h"
#include "util.h"
#include "modules/lang_lua/lang_lua.h"
#include "api.h"

MODULE_NAME("lua_core");
MODULE_AUTHOR("The Kalisko Team");
MODULE_DESCRIPTION("The lua_core module provides a Lua API to the Kalisko core");
MODULE_VERSION(0, 1, 10);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall_core", 0, 3, 2), MODULE_DEPENDENCY("lang_lua", 0, 5, 2));

MODULE_INIT
{
	GString *path;

	path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/lua_core/KaliskoModule.lua");

	if(!$(bool, lang_lua, evaluateLuaScript)(path->str)) {
		char *error = $(char *, lang_lua, popLuaString)();
		LOG_ERROR("Failed to run KaliskoModule.lua script: %s", error);
		free(error);
		g_string_free(path, true);
		return false;
	}

	g_string_free(path, true);

	path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/lua_core/KaliskoLog.lua");

	if(!$(bool, lang_lua, evaluateLuaScript)(path->str)) {
		char *error = $(char *, lang_lua, popLuaString)();
		LOG_ERROR("Failed to run KaliskoLog.lua script: %s", error);
		free(error);
		g_string_free(path, true);
		return false;
	}

	g_string_free(path, true);

	return true;
}

MODULE_FINALIZE
{

}
