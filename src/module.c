/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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

#define API
#include "log.h"
#include "memory_alloc.h"
#include "util.h"
#include "module.h"
#include "timer.h"

#ifdef WIN32
#define MODULE_PREFIX "kalisko_"
#define MODULE_SUFFIX ".dll"
#else
#define MODULE_PREFIX "libkalisko_"
#define MODULE_SUFFIX ".so"
#endif

#define MODULE_RELPATH "/modules/"

static GHashTable *modules;
static Module *core;
static char *modpath = NULL;

static bool checkModuleDependencyByModule(Module *source, Module *target);
static bool needModule(char *name, Version *needver, Module *parent);
static void unneedModule(void *name_p, void *mod_p, void *parent_p);
static bool unneedModuleWrapper(void *name_p, void *mod_p, void *parent_p);
static void *getLibraryFunction(Module *mod, char *funcname);
static bool loadDynamicLibrary(Module *mod, bool lazy);
static void unloadDynamicLibrary(Module *mod);

API void initModules()
{
	modules = g_hash_table_new(&g_str_hash, &g_str_equal);

	resetModuleSearchPath();

	// Construct our own symbolic core module
	core = allocateMemory(sizeof(Module));
	core->name = "core";
	core->description = "The Kalisko application framework core.";
	core->author = "The Kalisko team";
	core->version = createVersion(0, 0, 0, 0);
	core->bcversion = createVersion(0, 0, 0, 0);
	core->rc = 0;
	core->dependencies = g_hash_table_new(&g_str_hash, &g_str_equal);
	core->rdeps = g_hash_table_new(&g_str_hash, &g_str_equal);
	core->skip_reload = false;
	core->loaded = true;
}

API void freeModules()
{
	logMessage("core", LOG_LEVEL_NOTICE, "Revoking all modules...");
	g_hash_table_foreach_remove(core->dependencies, &unneedModuleWrapper, core);

	assert(g_hash_table_size(core->rdeps) == 0);
	assert(g_hash_table_size(modules) == 0);

	logMessage("core", LOG_LEVEL_NOTICE, "All modules successfully revoked");

	g_hash_table_destroy(modules);

	free(modpath);

	g_hash_table_destroy(core->dependencies);
	g_hash_table_destroy(core->rdeps);
	freeVersion(core->version);
	freeVersion(core->bcversion);
	free(core);
}

API char *getModuleSearchPath()
{
	return modpath;
}

API void setModuleSearchPath(char *path)
{
	if(modpath != NULL) {
		free(modpath);
	}

	if(path != NULL) {
		modpath = strdup(path);
	} else {
		modpath = NULL;
	}

#ifdef WIN32
	if(!SetDllDirectory(modpath)) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to set DLL directory to %s", modpath);
	}
#endif
}

API void resetModuleSearchPath()
{
	char *execpath = getExecutablePath();

	GString *temp = g_string_new(execpath);
	g_string_append(temp, MODULE_RELPATH);

	setModuleSearchPath(temp->str);

	g_string_free(temp, true);
	free(execpath);
}

API char *getModuleAuthor(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return mod->author;
}

API char *getModuleDescription(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return mod->description;
}

API Version *getModuleVersion(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return mod->version;
}

API Version *getModuleBcVersion(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return mod->bcversion;
}

API void *getModuleHandle(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) {
		return mod->handle;
	}

	return NULL;
}

API int getModuleReferenceCount(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return -1;
	}

	return mod->rc;
}

API GList *getModuleDependencies(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return g_hash_table_get_keys(mod->dependencies);
}

API GList *getModuleReverseDependencies(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(g_strcmp0(name, "core") == 0) {
		mod = core;
	}

	if(mod == NULL) {
		return NULL;
	}

	return g_hash_table_get_keys(mod->rdeps);
}

API GList *getActiveModules()
{
	return g_hash_table_get_keys(modules);
}

API bool isModuleLoaded(const char *name)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(mod == NULL) {
		return false;
	}

	return mod->loaded;
}

API bool isModuleRequested(const char *name)
{
	return g_hash_table_lookup(core->dependencies, name) != NULL;
}

API bool requestModule(char *name)
{
	if(g_strcmp0(name, "core") == 0) {
		logMessage("core", LOG_LEVEL_ERROR, "The Kalisko core can be neither requested nor revoked");
		return false;
	}

	if(g_hash_table_lookup(core->dependencies, name) != NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Cannot request already requested module %s", name);
		return false;
	}

	logMessage("core", LOG_LEVEL_NOTICE, "Requesting module %s", name);

	return needModule(name, NULL, core);
}

API bool revokeModule(char *name)
{
	if(g_strcmp0(name, "core") == 0) {
		logMessage("core", LOG_LEVEL_ERROR, "The Kalisko core can be neither requested nor revoked");
		return false;
	}

	Module *mod = g_hash_table_lookup(core->dependencies, name);

	if(mod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Cannot revoke unrequested module %s", name);
		return false;
	}

	logMessage("core", LOG_LEVEL_NOTICE, "Revoking module %s", name);

	// This needs to be done BEFORE unneedModule because modules enter themselves by their own name into the hashtables
	// and after unneedModule, their names are freed
	if(!g_hash_table_remove(core->dependencies, name)) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to remove %s from root set", name);
		exit(EXIT_FAILURE);
	}

	unneedModule(name, mod, core);

	return true;
}

API bool forceUnloadModule(char *name)
{
	if(g_strcmp0(name, "core") == 0) {
		logMessage("core", LOG_LEVEL_ERROR, "The Kalisko core can be neither requested nor revoked");
		return false;
	}

	Module *mod = g_hash_table_lookup(modules, name);

	if(mod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Cannot revoke unloaded module %s", name);
		return false;
	}

	logMessage("core", LOG_LEVEL_NOTICE, "Force unloading module %s", name);

	GList *rdeps = g_hash_table_get_keys(mod->rdeps);

	for(GList *iter = rdeps; iter != NULL; iter = iter->next) {
		if(g_strcmp0(iter->data, "core") == 0) { // arrived at root set
			continue; // skip core
		}

		if(!forceUnloadModule(iter->data)) {
			return false;
		}
	}

	// Look up ourselves again
	mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) { // The module is still loaded but should not have anymore other modules depending on it
		return revokeModule(name);
	} else { // We got freed in the process, that's fine too
		return true;
	}
}

API bool addModuleRuntimeDependency(char *source, char *target)
{
	Module *srcmod = g_hash_table_lookup(modules, source);
	if(srcmod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to add runtime dependency to module '%s': No such module loaded", source);
		return false;
	}

	Module *destmod = g_hash_table_lookup(modules, target);
	if(destmod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to add runtime dependency on module '%s': No such module loaded", target);
		return false;
	}

	if(checkModuleDependencyByModule(srcmod, destmod)) { // src already depends on dest
		logMessage("core", LOG_LEVEL_WARNING, "Trying to add already existing runtime dependency from module '%s' to '%s', skipping", source, target);
		return true;
	}

	if(checkModuleDependencyByModule(destmod, srcmod)) { // dest already depends on src
		logMessage("core", LOG_LEVEL_ERROR, "Trying to add circular runtime dependency from module '%s' to '%s', aborting", source, target);
		return false;
	}

	// Add dependency
	g_hash_table_insert(srcmod->dependencies, destmod->name, destmod);
	// Add reverse dependency
	g_hash_table_insert(destmod->rdeps, srcmod->name, srcmod);
	// Increase reference count
	destmod->rc++;

	logMessage("core", LOG_LEVEL_NOTICE, "Added runtime dependency from module '%s' to '%s', now needed by %d %s", source, target, destmod->rc, destmod->rc > 1 ? "dependencies" : "dependency");
	return true;
}

API bool checkModuleDependency(char *source, char *target)
{
	Module *srcmod = g_hash_table_lookup(modules, source);
	if(srcmod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to check dependency for module '%s': No such module loaded", source);
		return false;
	}

	Module *destmod = g_hash_table_lookup(modules, target);
	if(destmod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to check dependency on module '%s': No such module loaded", target);
		return false;
	}

	return checkModuleDependencyByModule(srcmod, destmod);
}

API void *getLibraryFunctionByName(char *module_name, char *function_name)
{
	Module *mod = g_hash_table_lookup(modules, module_name);
	if(mod == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to fetch function '%s' of module '%s': No such module loaded", function_name, module_name);
		return NULL;
	}
	return getLibraryFunction(mod, function_name);
}

/**
 * Checks if a module depends on another module by directly specifying the module objects
 *
 * @param source		the source module to check for a dependency
 * @param target		the target module to check for a dependency
 * @result				true if source depends on target (directly or indirectly)
 */
static bool checkModuleDependencyByModule(Module *source, Module *target)
{
	GHashTableIter iter;
	char *name;
	Module *module;
	g_hash_table_iter_init(&iter, source->dependencies);
	while(g_hash_table_iter_next(&iter, (void *) &name, (void *) &module)) {
		if(module == target) { // one of the listed dependencies is our target module
			return true;
		}

		// This isn't the target, but the target could be among these module's dependencies
		if(checkModuleDependencyByModule(module, target)) { // check recursively
			return true;
		}
	}

	// No dependency found
	return false;
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

	if(mod->handle == NULL) {
		logMessage("core", LOG_LEVEL_WARNING, "Trying to retrieve function %s in unloaded library %s of module %s", funcname, mod->dlname, mod->name);
		return NULL;
	}

#ifdef WIN32
	if((func = GetProcAddress(mod->handle, funcname)) == NULL) {
#else
	if((func = dlsym(mod->handle, funcname)) == NULL) {
#endif
		logMessage("core", LOG_LEVEL_WARNING, "Function %s doesn't exist in library %s of module %s", funcname, mod->dlname, mod->name);
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

	logMessage("core", LOG_LEVEL_INFO, "Loading dynamic library %s of module %s", mod->dlname, mod->name);

#ifdef WIN32
	if((mod->handle = LoadLibrary(mod->dlname)) == NULL) {
		char *error = g_win32_error_message(GetLastError());
		logMessage("core", LOG_LEVEL_ERROR, "Failed to load dynamic library %s of module %s: %s", mod->dlname, mod->name, error);
		free(error);
		return false;
	}

	mod->skip_reload = true; // Windows can't do more than lazy loading, so skip next reload
#else
	int mode;

	if(lazy) {
		mode = RTLD_LAZY;
	} else {
		mode = RTLD_NOW | RTLD_GLOBAL;
	}

	if((mod->handle = dlopen(mod->dlname, mode)) == NULL) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to load dynamic library %s of module %s: %s", mod->dlname, mod->name, dlerror());
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

	if(mod->handle == NULL) {
		return;
	}

	logMessage("core", LOG_LEVEL_INFO, "Unloading dynamic library %s of module %s", mod->dlname, mod->name);

#ifdef WIN32
	if(FreeLibrary(mod->handle) == 0) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to unload dynamic library %s of module %s", mod->dlname, mod->name);
	}
#else
	if(dlclose(mod->handle) != 0) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to unload dynamic library %s of module %s: %s", mod->dlname, mod->name, dlerror());
	}
#endif

	mod->handle = NULL;
}

/**
 * Tells a module it's needed or load it if it isn't loaded yet
 *
 * @param name			the module's name
 * @param needversion	the needed version of the module
 * @param parent		the module's parent
 * @result				true if successful, false on error
 */
static bool needModule(char *name, Version *needversion, Module *parent)
{
	Module *mod = g_hash_table_lookup(modules, name);

	if(mod != NULL) {
		if(!mod->loaded) {
			logMessage("core", LOG_LEVEL_ERROR, "Circular dependency on module %s", mod->name);
			return false;
		}

		// Version checking
		if(needversion != NULL) { // NULL would mean that any version is okay
			if(compareVersions(needversion, mod->bcversion) < 0) {
				GString *ver = dumpVersion(mod->version);
				GString *bcver = dumpVersion(mod->bcversion);
				GString *needver = dumpVersion(needversion);
				logMessage("core", LOG_LEVEL_ERROR, "Loaded module %s %s is too new to satisfy dependency on version %s, only backwards compatible down to %s", mod->name, ver->str, needver->str, bcver->str);
				g_string_free(ver, TRUE);
				g_string_free(bcver, TRUE);
				g_string_free(needver, TRUE);
				return false;
			} else if(compareVersions(needversion, mod->version) > 0) {
				GString *ver = dumpVersion(mod->version);
				GString *needver = dumpVersion(needversion);
				logMessage("core", LOG_LEVEL_ERROR, "Loaded module %s %s is too old to satisfy dependency on version %s", mod->name, ver->str, needver->str);
				g_string_free(ver, TRUE);
				g_string_free(needver, TRUE);
				return false;
			}
		}

		mod->rc++;
		logMessage("core", LOG_LEVEL_INFO, "Module %s is now needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
	} else {
		logMessage("core", LOG_LEVEL_INFO, "Unloaded module %s needed, loading...", name);

		mod = allocateMemory(sizeof(Module));

		mod->name = strdup(name);
		mod->dependencies = g_hash_table_new(&g_str_hash, &g_str_equal);
		mod->rdeps = g_hash_table_new(&g_str_hash, &g_str_equal);
		mod->skip_reload = false;
		mod->rc = 1;
		mod->loaded = false;
		mod->handle = NULL;

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
			g_hash_table_destroy(mod->dependencies);
			g_hash_table_destroy(mod->rdeps);
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
				logMessage("core", LOG_LEVEL_ERROR, "Available module %s %s is too new to satisfy dependency on version %s (only backwards compatible down to %s), aborting load", mod->name, ver->str, needver->str, bcver->str);
				g_string_free(ver, TRUE);
				g_string_free(bcver, TRUE);
				g_string_free(needver, TRUE);
				meta_error = true;
			} else if(compareVersions(needversion, meta_version) > 0) {
				GString *ver = dumpVersion(meta_version);
				GString *needver = dumpVersion(needversion);
				logMessage("core", LOG_LEVEL_ERROR, "Available module %s %s is too old to satisfy dependency on version %s, aborting load", mod->name, ver->str, needver->str);
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
		logMessage("core", LOG_LEVEL_NOTICE, "Loading module %s %s by %s, compatible >= %s", mod->name, ver->str, mod->author, bcver->str);
		g_string_free(ver, TRUE);
		g_string_free(bcver, TRUE);

		// Load dependencies
		for(ModuleDependency *dep = meta_dependencies; dep->name != NULL; dep++) {
			GString *depver = dumpVersion(&dep->version);
			logMessage("core", LOG_LEVEL_INFO, "Module %s depends on %s %s, needing...", mod->name, dep->name, depver->str);
			g_string_free(depver, TRUE);

			// Need the dependency
			if(!needModule(dep->name, &dep->version, mod)) {
				// Propagate down loading failure
				unneedModule(mod->name, mod, parent);
				return false;
			}
		}

		// Unload lazy loaded dynamic library
		unloadDynamicLibrary(mod);

		// Load dynamic library
		if(!loadDynamicLibrary(mod, false)) {
			unneedModule(mod->name, mod, parent);
			return false;
		}

		// Initialize module
		if((init_func = getLibraryFunction(mod, MODULE_INITIALIZER_FUNC)) == NULL) {
			unneedModule(mod->name, mod, parent);
			return false;
		}

		// Call module initializer
		logMessage("core", LOG_LEVEL_INFO, "Initializing module %s", mod->name);
		if(!init_func()) {
			logMessage("core", LOG_LEVEL_ERROR, "Failed to initialize module %s", mod->name);
			unneedModule(mod->name, mod, parent);
			return false;
		}

		mod->loaded = true; // finished loading

		logMessage("core", LOG_LEVEL_NOTICE, "Module %s loaded", mod->name);
	}

	// Add dependency
	g_hash_table_insert(parent->dependencies, mod->name, mod);
	// Add reverse dependency
	g_hash_table_insert(mod->rdeps, parent->name, parent);

	return true;
}

/**
 * A GHFunc that tells a module that it's no longer needed by its parent
 *
 * @param name		the module's name
 * @param mod_p		the module
 * @param parent_p	the module's parent by which it is no longer needed
 */
static void unneedModule(void *name, void *mod_p, void *parent_p)
{
	Module *mod = mod_p;
	Module *parent = parent_p;

	assert(mod != NULL);
	assert(parent != NULL);
	assert(strcmp(name, mod->name) == 0);

	// Remove reverse dependency
	g_hash_table_remove(mod->rdeps, parent->name);

	mod->rc--;

	if(mod->rc > 0) {
		logMessage("core", LOG_LEVEL_INFO, "Module %s is still needed by %d other %s", mod->name, mod->rc, mod->rc > 1 ? "dependencies" : "dependency");
		return;
	}

	logMessage("core", LOG_LEVEL_INFO, "Module %s is no longer needed, unloading...", mod->name);

	if(mod->loaded) { // Only finalize modules that are fully loaded
		// Call module finalizer
		logMessage("core", LOG_LEVEL_INFO, "Finalizing module %s", mod->name);
		ModuleFinalizer *finalize_func;

		if((finalize_func = getLibraryFunction(mod, MODULE_FINALIZER_FUNC)) != NULL) {
			finalize_func(); // Call finalizer if it exists
		}

		// Remove orphaned timers for this module
		int count = removeModuleTimers(mod->name);

		if(count > 0) {
			logMessage("core", LOG_LEVEL_INFO, "Removed %d orphaned timers from module %s", count, mod->name);
		}
	}

	// Unload dynamic library
	unloadDynamicLibrary(mod);

	// Remove ourselves from the modules table
	if(!g_hash_table_remove(modules, mod->name)) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to remove %s from modules table", mod->name);
		exit(EXIT_FAILURE);
	}

	logMessage("core", LOG_LEVEL_NOTICE, "Module %s unloaded", mod->name);

	// Tell our dependencies they are no longer needed
	g_hash_table_foreach_remove(mod->dependencies, &unneedModuleWrapper, mod);

	// Now the module's data structures
	g_hash_table_destroy(mod->dependencies);
	g_hash_table_destroy(mod->rdeps);
	free(mod->author);
	free(mod->description);
	free(mod->version);
	free(mod->bcversion);
	free(mod->name);
	free(mod->dlname);
	free(mod);
}

/**
 * A GHRFunc wrapper around unneedModule
 *
 * @param name_p	the module's name
 * @param mod_p		the module
 * @param parent_p	the module's parent by which it is no longer needed
 * @result			always true
 */
static bool unneedModuleWrapper(void *name_p, void *mod_p, void *parent_p)
{
	unneedModule(name_p, mod_p, parent_p);
	return true;
}
