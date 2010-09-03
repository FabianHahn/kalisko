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
#include "module.h"
#include "types.h"
#include "modules/config/config.h"
#include "modules/config/util.h"
#include "modules/getopts/getopts.h"
#include "modules/store/store.h"

#include "api.h"


MODULE_NAME("module_unload");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module reads a list from standard configuration to unload specific modules");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("config", 0, 3, 0), MODULE_DEPENDENCY("getopts", 0, 1, 0));

#define UNLOAD_CONFIG_PATH "unloadModules"

MODULE_INIT
{
	// Check for CLI options
	char *moduleList = $(char *, getopts, getOptValue)("unload-module", "u", NULL);

	if(moduleList) {
		LOG_INFO("Unloading modules given by command line argument");

		char **modules = g_strsplit(moduleList, ",", -1);

		for(int i = 0; modules[i] != NULL; i++) {
			char *module = modules[i];
			$$(bool, revokeModule)(module);
		}
	} else {
		Store *modules = $(Store *, config, getConfigPath)(UNLOAD_CONFIG_PATH);
		if(modules) {
			if(modules->type == STORE_STRING) {
				$$(bool, revokeModule)(modules->content.string);
			} else if(modules->type == STORE_LIST) {
				GQueue *list = modules->content.list;

				for(GList *iter = list->head; iter != NULL; iter = iter->next) {
					Store *moduleName = (Store *)iter->data;

					if(moduleName->type != STORE_STRING) {
						LOG_WARNING("Failed to read module unload list element because it is not a string. Skipping element.");
						continue;
					}

					$$(bool, revokeModule)(moduleName->content.string);
				}
			} else {
				LOG_WARNING("Unload modules configuration must be a string or a list of strings. Could not read configuration.")
			}
		}
	}

	return true;
}

MODULE_FINALIZE
{

}
