/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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

#include "dll.h"
#include "types.h"
#include "modules/cli_help/cli_help.h"
#include "modules/module_util/module_util.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "util.h"

#define API
#include "cli_help_meta.h"


MODULE_NAME("cli_help_meta");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Provides CLI Help by reading Store files.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("cli_help", 0, 1, 0), MODULE_DEPENDENCY("module_util", 0, 1, 0), MODULE_DEPENDENCY("store", 0, 5, 3));

MODULE_INIT
{
	// Default behavior: We load the help for all active modules
	char *modulePath = $$(char *, getModuleSearchPath)();
	GList *activeModules = $$(GList *, getActiveModules)();

	for(GList *moduleEntry = activeModules; moduleEntry != NULL; moduleEntry = moduleEntry->next) {
		char *metaFilePath = g_build_path("/", modulePath, (char *)moduleEntry->data, "meta.cfg", NULL);

		if(g_file_test(metaFilePath, G_FILE_TEST_EXISTS)) {
			loadCliHelpMetaFile(metaFilePath);
		}

		free(metaFilePath);
	}

	g_list_free(activeModules);

	return true;
}

MODULE_FINALIZE
{

}

API bool loadCliHelpMetaFile(char *filePath)
{
	Store *store = $(Store *, store, parseStoreFile)(filePath);
	if(store == NULL) {
		logNotice("Given store to parse for CLI Help cannot be loaded: %s", filePath);
		return false;
	}

	// first we load the CLI parameter help information
	Store *optHelp = $(Store *, store, getStorePath)(store, "cliHelp/options");
	if(optHelp == NULL) {
		logInfo("Given store has no CLI options help settings: %s", filePath);
	} else {
		if(optHelp->type != STORE_LIST) {
			logNotice("Given store has CLI options help but it is not a list. Ignoring.");
		} else {
			for(GList *current = optHelp->content.list->head; current != NULL; current = current->next) {
				// Load all possible values
				Store *valueStore = current->data;
				Store *valueModule = $(Store *, store, getStorePath)(valueStore, "module");
				Store *valueShort = $(Store *, store, getStorePath)(valueStore, "short");
				Store *valueLong = $(Store *, store, getStorePath)(valueStore, "long");
				Store *valueHelp = $(Store *, store, getStorePath)(valueStore, "help");

				// check that we can actually work with the values
				if(valueModule == NULL || valueHelp == NULL || (valueShort == NULL && valueLong == NULL)) {
					logNotice("CLI options help must include the module name, the help itself and the long or short parameter");
					continue;
				}

				if(valueModule->type != STORE_STRING) {
					logNotice("CLI options help provides the module name but it is not a string. Ignoring.");
					continue;
				}

				if(valueShort != NULL && valueShort->type != STORE_STRING) {
					logNotice("CLI options help provides the short parameter but it is not a string. Ignoring.");
					continue;
				}

				if(valueLong != NULL && valueLong->type != STORE_STRING) {
					logNotice("CLI options help provides the long paramter but it is not a string. Ignoring.");
					continue;
				}

				if(valueHelp->type != STORE_STRING) {
					logNotice("CLI options help provides a help but it is not a string. Ignoring.");
					continue;
				}

				// create the help entry
				char *shortParam = valueShort == NULL ? NULL : valueShort->content.string;
				char *longParam = valueLong == NULL ? NULL: valueLong->content.string;

				$(void, cli_help, addCLOptionHelp)(valueModule->content.string, shortParam, longParam, valueHelp->content.string);
			}
		}
	}

	// now we load the CLI arguments help information
	Store *argHelp = $(Store *, store, getStorePath)(store, "cliHelp/arguments");
	if(argHelp == NULL) {
		logInfo("Given store has no CLI arguments help settings: %s", filePath);
	} else {
		if(argHelp->type != STORE_LIST) {
			logNotice("Given store has CLI arguments help but it is not a list. Ignoring.");
		} else {
			for(GList *current = argHelp->content.list->head; current != NULL; current = current->next) {
				// Load all possible values
				Store *valueStore = current->data;
				Store *valueModule = $(Store *, store, getStorePath)(valueStore, "module");
				Store *valueName = $(Store *, store, getStorePath)(valueStore, "name");
				Store *valueHelp = $(Store *, store, getStorePath)(valueStore, "help");

				// check that we can actually work with the values
				if(valueModule == NULL || valueHelp == NULL || valueName == NULL) {
					logNotice("CLI argument help must contain a module, name and help key. Ignoring.");
					continue;
				}

				if(valueModule->type != STORE_STRING) {
					logNotice("CLI argument help provides the module name but it is not a string. Ignoring.");
					continue;
				}

				if(valueName->type != STORE_STRING) {
					logNotice("CLI argument help provides the name of the argument but it is not a string. Ignoring.");
				}

				if(valueHelp->type != STORE_STRING) {
					logNotice("CLI argument help provides a help but it is not a string. Ignoring.");
					continue;
				}

				// create the help entry
				$(void, cli_help, addCLArgumentHelp)(valueModule->content.string, valueName->content.string, valueHelp->content.string);
			}
		}
	}

	// clean up
	$(void, store, freeStore)(store);

	return true;
}
