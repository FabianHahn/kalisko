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
#include "log.h"
#include "types.h"
#include "timer.h"

#include "api.h"
#include "module_util.h"

MODULE_NAME("module_util");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Utility module offering functions to handle Kalisko modules");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

TIMER_CALLBACK(REQUEST_SELF);
TIMER_CALLBACK(SAFE_REMOVE_MODULE);
TIMER_CALLBACK(SAFE_FORCE_UNLOAD_MODULE);
TIMER_CALLBACK(SAFE_FORCE_RELOAD_MODULE);

MODULE_INIT
{
	TIMER_ADD_TIMEOUT(0, REQUEST_SELF);

	return true;
}

MODULE_FINALIZE
{

}

TIMER_CALLBACK(REQUEST_SELF)
{
	if(!$$(bool, isModuleRequested)("module_util")) {
		// Request ourselves to prevent being unloaded by a garbage collecting revoke call
		$$(bool, requestModule)("module_util");
	}
}

/**
 * Safely revokes a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic revokeModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to revoke
 */
API void safeRevokeModule(char *name)
{
	char *module = strdup(name);
	TIMER_ADD_TIMEOUT_EX(0, SAFE_REMOVE_MODULE, module);
}

TIMER_CALLBACK(SAFE_REMOVE_MODULE)
{
	char *module = (char *) custom_data;

	if(g_strcmp0(module, "module_util") != 0 && $$(bool, revokeModule)(module)) {
		LOG_INFO("Safely revoked module %s", module);
	} else {
		LOG_WARNING("Safe revoking of module %s failed", module);
	}

	free(module);
}

/**
 * Safely force unloads a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic forceUnloadModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to revoke
 */
API void safeForceUnloadModule(char *name)
{
	char *module = strdup(name);
	TIMER_ADD_TIMEOUT_EX(0, SAFE_FORCE_UNLOAD_MODULE, module);
}

TIMER_CALLBACK(SAFE_FORCE_UNLOAD_MODULE)
{
	char *module = (char *) custom_data;

	if(g_strcmp0(module, "module_util") != 0 && $$(bool, forceUnloadModule)(module)) {
		LOG_INFO("Safely force unloaded module %s", module);
	} else {
		LOG_WARNING("Safe force unloading of module %s failed", module);
	}

	free(module);
}

/**
 * Safely force reloads a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic forceUnloadModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to reload
 */
API void safeForceReloadModule(char *name)
{
	char *module = strdup(name);
	TIMER_ADD_TIMEOUT_EX(0, SAFE_FORCE_RELOAD_MODULE, module);
}

TIMER_CALLBACK(SAFE_FORCE_RELOAD_MODULE)
{
	char *module = (char *) custom_data;

	if(g_strcmp0(module, "module_util") != 0 && $$(bool, forceUnloadModule)(module)) {
		if($$(bool, requestModule)(module)) {
			LOG_INFO("Safely force reloaded module %s", module);
		} else {
			LOG_WARNING("Safe force reloading of module %s failed", module);
		}
	} else {
		LOG_WARNING("Safe force unloading of module %s failed", module);
	}

	free(module);
}
