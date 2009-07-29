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


#ifdef WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h> // readlink
#endif
#include <assert.h>
#include <stdlib.h>
#include <glib.h>
#include "api.h"
#include "log.h"
#include "memory_alloc.h"
#include "util.h"
#include "module.h"

#ifdef WIN32
#define MODULE_PREFIX ""
#define MODULE_SUFFIX ".dll"
#else
#define MODULE_PREFIX "lib"
#define MODULE_SUFFIX ".so"
#endif

#define MODULE_RELPATH "/modules/"

static GHashTable *modules;
static GHashTable *root_set;
static char *modpath;

static bool needModule(char *name, GHashTable *parent);
static void unneedModule(void *name_p, void *mod_p, void *data);
static gboolean unneedModuleWrapper(void *name_p, void *mod_p, void *data);
static void *getLibraryFunction(Module *mod, char *funcname);
static bool loadDynamicLibrary(Module *mod, bool lazy);
static void unloadDynamicLibrary(Module *mod);

/**
 * Initializes the modules data structures
 */
API void initModules()
{
	modules = g_hash_table_new(&g_str_hash, &g_str_equal);
	root_set = g_hash_table_new(&g_str_hash, &g_str_equal);

	char *execpath = getExecutablePath();

	GString *temp = g_string_new(execpath);
	g_string_append(temp, MODULE_RELPATH);

	modpath = temp->str;

	g_string_free(temp, FALSE);
	free(execpath);

#ifdef WIN32
	if(!SetDllDirectory(modpath)) {
		logMessage(LOG_TYPE_ERROR, "Failed to set DLL directory to %s", modpath);
	}
#endif
}

/**
 * Frees the modules data structures
 */
API void freeModules()
{
	logMessage(LOG_TYPE_INFO, "Revoking all modules...");
	g_hash_table_foreach_remove(root_set, &unneedModuleWrapper, NULL);

	assert(g_hash_table_size(root_set) == 0);
	assert(g_hash_table_size(modules) == 0);

	logMessage(LOG_TYPE_INFO, "All modules successfully revoked");

	g_hash_table_destroy(root_set);
	g_hash_table_destroy(modules);

	free(modpath);
}

/**
 * Requests a module
 *
 * @param name		the module's name
 * @result			true if successful, false on error
 */
API bool requestModule(char *name)
{
	if(g_hash_table_lookup(root_set, name) != NULL) {
		logMessage(LOG_TYPE_ERROR, "Cannot request already requested module %s", name);
		return false;
	}

	logMessage(LOG_TYPE_INFO, "Requesting module %s", name);

	return needModule(name, root_set);
}

/**
 * Revokes a module
 *
 * @param name		the module to revoke
 * @result			true if successful, false on error
 */
API bool revokeModule(char *name)
{
	Module *mod = g_hash_table_lookup(root_set, name);

	if(mod == NULL) {
		logMessage(LOG_TYPE_ERROR, "Cannot revoke unrequested module %s", name);
		return false;
	}

	logMessage(LOG_TYPE_INFO, "Revoking module %s", name);

	// This needs to be done BEFORE unneedModule because modules enter themselves by their own name into the hashtables
	// and after unneedModule, their names are freed
	if(!g_hash_table_remove(root_set, name)) {
		logMessage(LOG_TYPE_ERROR, "Failed to remove %s from root set", name);
		exit(EXIT_FAILURE);
	}

	unneedModule(name, mod, NULL);

	return true;
}

/**
 * Fetches a function from a dynamic library of a module
 *
 * @param mod			the module
 * @param funcname		the name of the function to fetch
 * @result				the function or NULL if not found
 */
static void *getLibraryFunction(Module *mod, char *funcname)
{
	void *func;

#ifdef WIN32
	if((func = GetProcAddress(mod->handle, funcname)) == NULL) {
#else
	if((func = dlsym(mod->handle, funcname)) == NULL) {
#endif
		logMessage(LOG_TYPE_ERROR, "Function %s doesn't exist in library %s of module %s", funcname, mod->dlname, mod->name);
		return NULL;
	}

	return func;
}

/**
 * Loads the shared library associated with a module
 *
 * @param mod			the module
 * @param lazy			should the library be loaded lazily?
 * @result				true if successful, false on error
 */
static bool loadDynamicLibrary(Module *mod, bool lazy)
{
	if(mod->skip_reload) {
		mod->skip_reload = false;
		return true;
	}

	logMessage(LOG_TYPE_DEBUG, "Loading dynamic library %s of module %s", mod->dlname, mod->name);

#ifdef WIN32
	if((mod->handle = LoadLibrary(mod->dlname)) == NULL) {
		logMessage(LOG_TYPE_ERROR, "Failed to load dynamic library %s of module %s", mod->dlname, mod->name);
		return false;
	}

	mod->skip_reload = true; // Windows can't do more than lazy loading, so skip next reload
#else
	int mode;

	if(lazy) {
		mode = RTLD_LAZY;
	} else {
		mode = RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND;
	}

	if((mod->handle = dlopen(mod->dlname, mode)) == NULL) {
		logMessage(LOG_TYPE_ERROR, "Failed to load dynamic library %s of module %s: %s", mod->dlname, mod->name, dlerror());
		return false;
	}
#endif

	return true;
}

/**
 * Unloads the dynamic library associated with a module
 *
 * @param mod			the module
 */
static void unloadDynamicLibrary(Module *mod)
{
	if(mod->skip_reload) {
		return;
	}

	logMessage(LOG_TYPE_DEBUG, "Unloading dynamic library %s of module %s", mod->dlname, mod->name);

#ifdef WIN32
	if(FreeLibrary(mod->handle) == 0) {
		logMessage(LOG_TYPE_ERROR, "Failed to unload dynamic library %s of module %s", mod->dlname, mod->name);
	}
#else
	if(dlclose(mod->handle) != 0) {
		logMessage(LOG_TYPE_ERROR, "Failed to unload dynamic library %s of module %s: %s", mod->dlname, mod->name, dlerror());
	}
#endif

	mod->handle = NULL;
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
	Module *mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) {
		mod->rc++;
		logMessage(LOG_TYPE_DEBUG, "Module %s is now needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
	} else {
		logMessage(LOG_TYPE_INFO, "Unloaded module %s needed, loading...", name);

		mod = allocateMemory(sizeof(Module));

		mod->name = strdup(name);
		mod->dependencies = g_hash_table_new(&g_str_hash, &g_str_equal);
		mod->skip_reload = false;
		mod->rc = 1;
		mod->loaded = false;

		GString *libname = g_string_new(mod->name);
		g_string_prepend(libname, MODULE_PREFIX);
		g_string_prepend(libname, modpath);
		g_string_append(libname, MODULE_SUFFIX);
		mod->dlname = libname->str;
		g_string_free(libname, FALSE);

		g_hash_table_insert(modules, mod->name, mod);

		// Lazy-load dynamic library
		if(!loadDynamicLibrary(mod, true)) {
			g_hash_table_remove(modules, mod->name);
			free(mod->name);
			free(mod->dlname);
			free(mod);
			return false;
		}

		// Fetch dependencies
		ModuleDepender *depends_func;
		if((depends_func = getLibraryFunction(mod, "module_depends")) == NULL) {
			g_hash_table_remove(modules, mod->name);
			free(mod->name);
			free(mod->dlname);
			free(mod);
			return false;
		}

		GList *deplist = depends_func();

		// Load dependencies
		for(GList *iter = deplist; iter != NULL; iter = iter->next) {
			logMessage(LOG_TYPE_DEBUG, "Module %s depends on %s, needing...", mod->name, iter->data);

			Module *depmod = g_hash_table_lookup(modules, iter->data);

			if(depmod != NULL && !depmod->loaded) {
				logMessage(LOG_TYPE_ERROR, "Circular dependency on module %s, requested by %s", iter->data, mod->name);
				unneedModule(mod->name, mod, NULL); // Recover, unneed self and all already loaded dependencies
				return false;
			}

			// Need the dependency
			if(!needModule(iter->data, mod->dependencies)) { // Propagate down circular recovery
				unneedModule(mod->name, mod, NULL);
				return false;
			}
		}

		g_list_free(deplist);

		// Unload lazy loaded dynamic library
		unloadDynamicLibrary(mod);

		// Load dynamic library
		if(!loadDynamicLibrary(mod, false)) {
			unneedModule(mod->name, mod, NULL);
			return false;
		}

		// Check library functions
		if(getLibraryFunction(mod, "module_init") == NULL || getLibraryFunction(mod, "module_finalize") == NULL) {
			unneedModule(mod->name, mod, NULL);
			return false;
		}

		// Call module initializer
		logMessage(LOG_TYPE_DEBUG, "Initializing module %s", mod->name);
		ModuleInitializer *init_func = getLibraryFunction(mod, "module_init");
		if(!init_func()) {
			logMessage(LOG_TYPE_ERROR, "Failed to initialize module %s\n");
			unneedModule(mod->name, mod, NULL);
			return false;
		}

		mod->loaded = true; // finished loading

		logMessage(LOG_TYPE_INFO, "Module %s loaded", mod->name);
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
	Module *mod = mod_p;

	assert(mod != NULL);
	assert(strcmp(name, mod->name) == 0);

	mod->rc--;

	if(mod->rc > 0) {
		logMessage(LOG_TYPE_DEBUG, "Module %s is still needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
		return;
	}

	logMessage(LOG_TYPE_INFO, "Module %s is no longer needed, unloading...", mod->name);

	// Module is obsolete
	g_hash_table_foreach(mod->dependencies, &unneedModule, NULL);

	// Call module finalizer
	logMessage(LOG_TYPE_DEBUG, "Finalizing module %s", mod->name);
	ModuleFinalizer *finalize_func = getLibraryFunction(mod, "module_finalize");
	finalize_func();

	// Unload dynamic library
	unloadDynamicLibrary(mod);

	if(!g_hash_table_remove(modules, mod->name)) {
		logMessage(LOG_TYPE_ERROR, "Failed to remove %s from modules table", mod->name);
		exit(EXIT_FAILURE);
	}

	g_hash_table_destroy(mod->dependencies);

	// free(mod->ver); TODO add version freeing as soon as its fetched somewhere

	logMessage(LOG_TYPE_INFO, "Module %s unloaded", mod->name);

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
