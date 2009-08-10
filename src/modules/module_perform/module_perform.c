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
#include "modules/config/config.h"
#include "modules/config_standard/util.h"

#include "api.h"

#define CONFIG_PATH "loadModules"

MODULE_NAME("module_perform");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The perform module loads other user-defined modules from the standard config upon startup");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config_standard", 0, 1, 0));

MODULE_INIT
{
	ConfigNodeValue *modules = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)(CONFIG_PATH);
	if(modules != NULL) {
		if(modules->type != CONFIG_LIST) {
			LOG_WARNING("In standard configurations '%s' must be a list", CONFIG_PATH);
			return false;
		}

		for(int moduleNameIndex = 0; moduleNameIndex < modules->content.list->length; moduleNameIndex++) {
			ConfigNodeValue *moduleName = g_queue_peek_nth(modules->content.list, moduleNameIndex);

			if(moduleName->type != CONFIG_STRING) {
				LOG_WARNING("Each value of loadModules must be a string value");
				continue;
			}

			if(!$$(bool, requestModule)(moduleName->content.string)) {
				LOG_WARNING("Module Perform could not request '%s' module", moduleName->content.string);
			} else {
				LOG_DEBUG("Module Perform requested successfully '%s'", moduleName->content.string);
			}
		}
	}

	return true;
}

MODULE_FINALIZE
{
}
