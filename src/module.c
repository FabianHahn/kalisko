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
#define MODULE_PREFIX "kalisko_"
#define MODULE_SUFFIX ".dll"
#else
#define MODULE_PREFIX "libkalisko_"
#define MODULE_SUFFIX ".so"
#endif

#define MODULE_RELPATH "/modules/"

static GHashTable *modules;
static GHashTable *root_set;
static char *modpath;

static bool needModule(char *name, Version *needver, GHashTable *parent);
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

	return needModule(name, NULL, root_set);
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
 * @param name			the module's name
 * @param needversion	the needed version of the module
 * @param parent		the module's parent tree
 * @result				true if successful, false on error
 */
static bool needModule(char *name, Version *needversion, GHashTable *parent)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) {
		if(!mod->loaded) {
			logMessage(LOG_TYPE_ERROR, "Circular dependency on module %s", mod->name);
			return false;
		}

		// Version checking
		if(needversion != NULL) { // NULL would mean that any version is okay
			if(compareVersions(needversion, mod->bcversion) < 0) {
				GString *ver = dumpVersion(mod->version);
				GString *bcver = dumpVersion(mod->bcversion);
				GString *needver = dumpVersion(needversion);
				logMessage(LOG_TYPE_ERROR, "Loaded module %s %s is too new to satisfy dependency on version %s, only backwards compatible down to %s", mod->name, ver->str, needver->str, bcver->str);
				g_string_free(ver, TRUE);
				g_string_free(bcver, TRUE);
				g_string_free(needver, TRUE);
				return false;
			} else if(compareVersions(needversion, mod->version) > 0) {
				GString *ver = dumpVersion(mod->version);
				GString *needver = dumpVersion(needversion);
				logMessage(LOG_TYPE_ERROR, "Loaded module %s %s is too old to satisfy dependency on version %s", mod->name, ver->str, needver->str);
				g_string_free(ver, TRUE);
				g_string_free(needver, TRUE);
				return false;
			}
		}

		mod->rc++;
		logMessage(LOG_TYPE_DEBUG, "Module %s is now needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
	} else {
		logMessage(LOG_TYPE_DEBUG, "Unloaded module %s needed, loading...", name);

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

		// Insert the newly created module into the module table
		g_hash_table_insert(modules, mod->name, mod);

		// Lazy-load dynamic library
		if(!loadDynamicLibrary(mod, true)) {
			g_hash_table_remove(modules, mod->name);
			free(mod->name);
			free(mod->dlname);
			free(mod);
			return false;
		}

		ModuleDescriptor *descriptor_func;
		ModuleVersioner *version_func;
		ModuleDepender *depender_func;
		ModuleInitializer *init_func;
		bool meta_error = false;
		char *meta_author = NULL;
		char *meta_description = NULL;
		Version *meta_version = NULL;
		Version *meta_bcversion = NULL;
		ModuleDependency *meta_dependencies;

		if((descriptor_func = getLibraryFunction(mod, MODULE_NAME_FUNC)) == NULL) {
			meta_error = true;
		} else {
			// Check if meta name matches module name
			meta_error = meta_error && strcmp(mod->name, descriptor_func());
		}

		// Fetch module author
		if((descriptor_func = getLibraryFunction(mod, MODULE_AUTHOR_FUNC)) == NULL) {
			meta_error = true;
		} else {
			meta_author = descriptor_func();
		}

		// Fetch module description
		if((descriptor_func = getLibraryFunction(mod, MODULE_DESCRIPTION_FUNC)) == NULL) {
			meta_error = true;
		} else {
			meta_description = descriptor_func();
		}

		// Fetch module version
		if((version_func = getLibraryFunction(mod, MODULE_VERSION_FUNC)) == NULL) {
			meta_error = true;
		} else {
			meta_version = version_func();
		}

		// Fetch module bcversion
		if((version_func = getLibraryFunction(mod, MODULE_BCVERSION_FUNC)) == NULL) {
			meta_error = true;
		} else {
			meta_bcversion = version_func();
		}

		// Version checking
		if(needversion != NULL && meta_version != NULL && meta_bcversion != NULL) { // NULL would mean that any version is okay
			if(compareVersions(needversion, meta_bcversion) < 0) {
				GString *ver = dumpVersion(meta_version);
				GString *bcver = dumpVersion(meta_bcversion);
				GString *needver = dumpVersion(needversion);
				logMessage(LOG_TYPE_ERROR, "Available module %s %s is too new to satisfy dependency on version %s (only backwards compatible down to %s), aborting load", mod->name, ver->str, needver->str, bcver->str);
				g_string_free(ver, TRUE);
				g_string_free(bcver, TRUE);
				g_string_free(needver, TRUE);
				meta_error = true;
			} else if(compareVersions(needversion, meta_version) > 0) {
				GString *ver = dumpVersion(meta_version);
				GString *needver = dumpVersion(needversion);
				logMessage(LOG_TYPE_ERROR, "Available module %s %s is too old to satisfy dependency on version %s, aborting load", mod->name, ver->str, needver->str);
				g_string_free(ver, TRUE);
				g_string_free(needver, TRUE);
				meta_error = true;
			}
		}

		// Fetch dependencies
		if((depender_func = getLibraryFunction(mod, MODULE_DEPENDS_FUNC)) == NULL) {
			meta_error = true;
		} else {
			meta_dependencies = depender_func();
		}

		if(meta_error) {
			g_hash_table_remove(modules, mod->name);
			free(mod->name);
			free(mod->dlname);
			free(mod);
			return false;
		}

		mod->author = strdup(meta_author);
		mod->description = strdup(meta_description);
		mod->version = copyVersion(meta_version);
		mod->bcversion = copyVersion(meta_bcversion);

		GString *ver = dumpVersion(mod->version);
		GString *bcver = dumpVersion(mod->bcversion);
		logMessage(LOG_TYPE_INFO, "Loading module %s %s by %s, compatible >= %s", mod->name, ver->str, mod->author, bcver->str);
		g_string_free(ver, TRUE);
		g_string_free(bcver, TRUE);

		// Load dependencies
		for(ModuleDependency *dep = meta_dependencies; dep->name != NULL; dep++) {
			GString *depver = dumpVersion(&dep->version);
			logMessage(LOG_TYPE_DEBUG, "Module %s depends on %s %s, needing...", mod->name, dep->name, depver->str);
			g_string_free(depver, TRUE);

			// Need the dependency
			if(!needModule(dep->name, &dep->version, mod->dependencies)) {
				// Propagate down loading failure
				unneedModule(mod->name, mod, NULL);
				return false;
			}
		}

		// Unload lazy loaded dynamic library
		unloadDynamicLibrary(mod);

		// Load dynamic library
		if(!loadDynamicLibrary(mod, false)) {
			unneedModule(mod->name, mod, NULL);
			return false;
		}

		// Initialize module
		if((init_func = getLibraryFunction(mod, MODULE_INITIALIZER_FUNC)) == NULL) {
			unneedModule(mod->name, mod, NULL);
			return false;
		}

		// Call module initializer
		logMessage(LOG_TYPE_DEBUG, "Initializing module %s", mod->name);
		if(!init_func()) {
			logMessage(LOG_TYPE_ERROR, "Failed to initialize module %s");
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
 * @param mod_p		the module
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

	logMessage(LOG_TYPE_DEBUG, "Module %s is no longer needed, unloading...", mod->name);

	if(mod->loaded) { // Only finalize modules that are fully loaded
		// Call module finalizer
		logMessage(LOG_TYPE_DEBUG, "Finalizing module %s", mod->name);
		ModuleFinalizer *finalize_func;

		if((finalize_func = getLibraryFunction(mod, MODULE_FINALIZER_FUNC)) != NULL) {
			finalize_func(); // Call finalizer if it exists
		}
	}

	// Tell our dependencies they are no longer needed
	g_hash_table_foreach(mod->dependencies, &unneedModule, NULL);

	// Unload dynamic library
	unloadDynamicLibrary(mod);

	if(!g_hash_table_remove(modules, mod->name)) {
		logMessage(LOG_TYPE_ERROR, "Failed to remove %s from modules table", mod->name);
		exit(EXIT_FAILURE);
	}

	g_hash_table_destroy(mod->dependencies);

	free(mod->author);
	free(mod->description);
	free(mod->version);
	free(mod->bcversion);

	logMessage(LOG_TYPE_INFO, "Module %s unloaded", mod->name);

	free(mod->name);
	free(mod);
}

/**
 * A GHRFunc wrapper around unneedModule
 *
 * @param name_p	the module's name
 * @param mod_p		the module
 * @param data		unused
 * @result			always TRUE
 */
static gboolean unneedModuleWrapper(void *name_p, void *mod_p, void *data)
{
	unneedModule(name_p, mod_p, data);
	return TRUE;
}
