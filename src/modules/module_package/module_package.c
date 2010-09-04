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
#include "modules/config/util.h"
#include "modules/getopts/getopts.h"
#include "modules/store/store.h"

#include "api.h"


MODULE_NAME("module_package");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Loads modules of a given package from standard configruations");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("config", 0, 3, 0), MODULE_DEPENDENCY("getopts", 0, 1, 0));

static void loadPackage(char *package);

#define PACKAGE_PATH "packages"
#define USE_PACKAGE_PATH "usePackage"

MODULE_INIT
{
	char *usePackage = $(char *, getopts, getOptValue)("load-package", "l", NULL);

	if(usePackage) {
		LOG_INFO("Loading package given by command line argument: '%s'", usePackage);

		char **moduleList = g_strsplit(usePackage, ",", -1);
		for(int i = 0; moduleList[i] != NULL; i++) {
			loadPackage(moduleList[i]);
		}
	} else {
		Store *usePackageStore = $(Store *, config, getConfigPath)(USE_PACKAGE_PATH);
		if(usePackageStore != NULL) {
			if(usePackageStore->type == STORE_LIST) {
				GQueue *packageList = usePackageStore->content.list;

				for(GList *iter = packageList->head; iter != NULL; iter = iter->next) {
					Store *packageName = (Store *)iter->data;

					if(packageName->type != STORE_STRING) {
						LOG_WARNING("Package loading failed: '%s' must contain only strings", USE_PACKAGE_PATH);
						continue;
					}

					loadPackage(packageName->content.string);
				}
			} else if(usePackageStore->type == STORE_STRING) {
				loadPackage(usePackageStore->content.string);
			} else {
				LOG_WARNING("Package loading failed: '%s' must be a list or a string", USE_PACKAGE_PATH);
				return false;
			}

		} else {
			LOG_INFO("No package given to load.");
			return true;
		}
	}

	return true;
}

MODULE_FINALIZE
{

}

static void loadPackage(char *package)
{
	char *packagePath = g_build_path("/", PACKAGE_PATH, package, NULL);

	Store *moduleList = $(Store *, config, getConfigPath)(packagePath);
	if(moduleList) {
		if(moduleList->type != STORE_LIST) {
			LOG_WARNING("Package loading failed: Package '%s' cannot be loaded as the package is not a list", package);
		} else {
			GQueue *list = moduleList->content.list;
			for(GList *iter = list->head; iter != NULL; iter = iter->next) {
				Store *moduleName = (Store *)iter->data;

				if(moduleName->type != STORE_STRING) {
					LOG_WARNING("Package loading failed: Package '%s' contains non string values. All values must be strings. Ignoring value", package);
					continue;
				}

				if(!$$(bool, requestModule)(moduleName->content.string)) {
					LOG_WARNING("Module load failed to request module '%s' for package '%s'", moduleName->content.string, package);
				} else {
					LOG_DEBUG("Module '%s' loaded successfully for package '%s'", moduleName->content.string, package);
				}
			}
		}
	} else {
		LOG_WARNING("Package loading failed: Package '%s' does not exist", package);
	}

	free(packagePath);
}
