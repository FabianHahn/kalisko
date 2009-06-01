/**
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Kalisko Developers nor the names of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdlib.h>
#include <glib.h>
#include "log.h"
#include "module.h"
#include "memory_alloc.h"

static GHashTable *modules;
static GHashTable *root_set;

static bool needModule(char *name, GHashTable *parent);
static void unneedModule(void *name_p, void *mod_p, void *data);
static gboolean unneedModuleWrapper(void *name_p, void *mod_p, void *data);

/**
 * Initializes the modules data structures
 */
void initModules()
{
	modules = g_hash_table_new(&g_str_hash, &g_str_equal);
	root_set = g_hash_table_new(&g_str_hash, &g_str_equal);
}

/**
 * Frees the modules data structures
 */
void freeModules()
{
	logInfo("Revoking all modules...");
	g_hash_table_foreach_remove(root_set, &unneedModuleWrapper, NULL);

	assert(g_hash_table_size(root_set) == 0);
	assert(g_hash_table_size(modules) == 0);

	logInfo("All modules successfully revoked");

	g_hash_table_destroy(root_set);
	g_hash_table_destroy(modules);
}

/**
 * Requests a module
 *
 * @param name		the module's name
 * @result			true if successful, false on error
 */
bool requestModule(char *name)
{
	if(g_hash_table_lookup(root_set, name) != NULL) {
		logError("Cannot request already requested module %s", name);
		return false;
	}

	logInfo("Requesting module %s", name);

	return needModule(name, root_set);
}

/**
 * Revokes a module
 *
 * @param name		the module to revoke
 * @result			true if successful, false on error
 */
bool revokeModule(char *name)
{
	module *mod = g_hash_table_lookup(root_set, name);

	if(mod == NULL) {
		logError("Cannot revoke unrequested module %s", name);
		return false;
	}

	logInfo("Revoking module %s", name);

	// This needs to be done BEFORE unneedModule because modules enter themselves by their own name into the hashtables
	// and after unneedModule, their names are freed
	if(!g_hash_table_remove(root_set, name)) {
		logError("Failed to remove %s from root set", name);
		exit(EXIT_FAILURE);
	}

	unneedModule(name, mod, NULL);

	return true;
}

/**
 * Tells a module it's needed or load it if it isn't loaded yet
 *
 * @param name		the module's name
 * @param parent	the module's parent tree
 * @result			true if successful, false on error
 */
static bool needModule(char *name, GHashTable *parent)
{
	module *mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) {
		mod->rc++;
		logDebug("Module %s is now needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
	} else {
		logInfo("Unloaded module %s needed, loading...", name);

		mod = allocateObject(module);

		mod->name = strdup(name);
		mod->dependencies = g_hash_table_new(&g_str_hash, &g_str_equal);
		mod->rc = 1;
		mod->loaded = false;

		g_hash_table_insert(modules, mod->name, mod);

		// TODO fetch actual dependencies instead of this hardcoded placeholder

		GList *deplist = NULL;

		if(strcmp(name, "test") == 0) {
			deplist = g_list_append(deplist, "dep1");
		} else if(strcmp(name, "dep1") == 0) {
			deplist = g_list_append(deplist, "dep2");
		} else if(strcmp(name, "test2") == 0) {
			deplist = g_list_append(deplist, "dep2");
		}

		for(GList *iter = deplist; iter != NULL; iter = iter->next) {
			logDebug("Module %s depends on %s, needing...", mod->name, iter->data);

			module *depmod = g_hash_table_lookup(modules, iter->data);

			if(depmod != NULL && !depmod->loaded) {
				logError("Circular dependency on module %s, requested by %s", iter->data, mod->name);
				unneedModule(mod->name, mod, NULL); // Recover, unneed self and all already loaded dependencies
				return false;
			}

			// Need the dependency
			if(!needModule(iter->data, mod->dependencies)) { // Propagate down circular recovery
				unneedModule(mod->name, mod, NULL);
				return false;
			}
		}

		// TODO actually load the module

		mod->loaded = true; // finished loading

		logInfo("Module %s loaded", mod->name);
	}

	g_hash_table_insert(parent, mod->name, mod);

	return true;
}

/**
 * A GHFunc that tells a module that it's no longer needed by its parent
 *
 * @param name		the module's name
 * @param mod		the module
 * @param data		unused
 */
static void unneedModule(void *name, void *mod_p, void *data)
{
	module *mod = mod_p;

	assert(mod != NULL);
	assert(strcmp(name, mod->name) == 0);

	mod->rc--;

	if(mod->rc > 0) {
		logDebug("Module %s is still needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
		return;
	}

	logInfo("Module %s is no longer needed, unloading...", mod->name);

	// Module is obsolete
	g_hash_table_foreach(mod->dependencies, &unneedModule, NULL);

	// TODO actual unloading mechanism

	if(!g_hash_table_remove(modules, mod->name)) {
		logError("Failed to remove %s from modules table", mod->name);
		exit(EXIT_FAILURE);
	}

	g_hash_table_destroy(mod->dependencies);

	// free(mod->ver); TODO add version freeing as soon as its fetched somewhere

	logInfo("Module %s unloaded", mod->name);

	free(mod->name);
	free(mod);
}

/**
 * A GHRFunc wrapper around unneedModule
 *
 * @param name		the module's name
 * @param mod		the module
 * @param data		unused
 * @result			always TRUE
 */
static gboolean unneedModuleWrapper(void *name_p, void *mod_p, void *data)
{
	unneedModule(name_p, mod_p, data);
	return TRUE;
}
