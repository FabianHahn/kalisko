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

#include <stdlib.h>
#include "dll.h"
#include "log.h"
#include "module.h"
#include "types.h"
#include "hooks.h"
#include "modules/config/config.h"
#include "modules/config/util.h"

#include "api.h"

#define PERFORM_CONFIG_PATH "loadModules"

MODULE_NAME("module_perform");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The perform module loads other user-defined modules from the standard config upon startup");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 0));

MODULE_INIT
{
	HOOK_ADD(module_perform_finished);

	LOG_INFO("Requesting perform modules");

	Store *modules = $(Store *, config, getConfigPath)(PERFORM_CONFIG_PATH);
	if(modules != NULL) {
		if(modules->type != STORE_LIST) {
			LOG_ERROR("Module perform failed: Standard configuration value '%s' must be a list", PERFORM_CONFIG_PATH);
			return false;
		}

		GQueue *list = modules->content.list;

		for(GList *iter = list->head; iter != NULL; iter = iter->next) {
			Store *moduleName = (Store *) iter->data;

			if(moduleName->type != STORE_STRING) {
				LOG_WARNING("Failed to read module perform entry: Every list value of '%s' must be a string", PERFORM_CONFIG_PATH);
				continue;
			}

			if(!$$(bool, requestModule)(moduleName->content.string)) {
				LOG_ERROR("Module perform failed to request module %s", moduleName->content.string);
			} else {
				LOG_DEBUG("Module perform successfully requested %s", moduleName->content.string);
			}
		}
	} else {
		LOG_DEBUG("Module perform does not have any modules to load.");
	}

	HOOK_TRIGGER(module_perform_finished);

	return true;
}

MODULE_FINALIZE
{
	HOOK_DEL(module_perform_finished);
}
