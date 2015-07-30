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


#ifndef MODULE_H
#define MODULE_H

#include <glib.h>
#include "version.h"
#include "types.h"
#include "memory_alloc.h"

typedef struct {
	char *name;
	char *dlname;
	char *author;
	char *description;
	Version *version;
	Version *bcversion;
	void *handle;
	int rc;
	GHashTable *dependencies;
	GHashTable *rdeps;
	bool loaded;
	bool skip_reload;
} Module;

typedef struct {
	char *name;
	Version version;
} ModuleDependency;

typedef bool (ModuleInitializer)();
typedef void (ModuleFinalizer)();
typedef Version *(ModuleVersioner)();
typedef ModuleDependency *(ModuleDepender)();
typedef char *(ModuleDescriptor)();


/**
 * Initializes the modules data structures
 */
API void initModules();

/**
 * Frees the modules data structures
 */
API void freeModules();

/**
 * Returns the current module search path
 *
 * @result			the current module search path, must not be changed
 */
API char *getModuleSearchPath();

/**
 * Sets the module search path to a different directory
 *
 * @param path		the new module search path to use or NULL to disable the search path
 */
API void setModuleSearchPath(char *path);

/**
 * Resets the module search path to its default value
 */
API void resetModuleSearchPath();

/**
 * Returns the module author of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module author or NULL if the module isn't at least loading
 */
API char *getModuleAuthor(const char *name);

/**
 * Returns the module description of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module description or NULL if the module isn't at least loading
 */
API char *getModuleDescription(const char *name);

/**
 * Returns the module version of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module version or NULL if the module isn't at least loading
 */
API Version *getModuleVersion(const char *name);

/**
 * Returns the module backwards compatible version of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module backwards compatible version or NULL if the module isn't at least loading
 */
API Version *getModuleBcVersion(const char *name);

/**
 * Returns the internal handle of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the handle of the module's shared libarary
 */
API void *getModuleHandle(const char *name);

/**
 * Returns the module reference count of a loaded module. A module is automatically unloaded once its reference count reaches zero
 *
 * @param name		the name of the module to check
 * @result			the module reference count or -1 if the module isn't at least loading
 */
API int getModuleReferenceCount(const char *name);

/**
 * Returns the module dependencies of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module dependencies or NULL if the module isn't at least loading, must not be modified but freed with g_list_free after use
 */
API GList *getModuleDependencies(const char *name);

/**
 * Returns the module reverse dependencies of a loaded module
 *
 * @param name		the name of the module to check
 * @result			the module reverse dependencies or NULL if the module isn't at least loading, must not be modified but freed with g_list_free after use
 */
API GList *getModuleReverseDependencies(const char *name);

/**
 * Returns a list of active modules. A module is considered active if it's either already loaded or currently loading
 *
 * @result			a list of active modules, must not be modified but freed with g_list_free after use
 */
API GList *getActiveModules();

/**
 * Checks whether a module with a given name is loaded. Note that modules currently loading are reported as not being loaded yet.
 *
 * @param name		the name of the module to check
 * @result			true if the module is loaded
 */
API bool isModuleLoaded(const char *name);

/**
 * Checks whether a module with a given name is requested. Note that modules currently loading are reported as not being requested yet.
 *
 * @param name		the name of the module to check
 * @result			true if the module is requested
 */
API bool isModuleRequested(const char *name);

/**
 * Requests a module
 *
 * @param name		the module's name
 * @result			true if successful, false on error
 */
API bool requestModule(char *name);

/**
 * Revokes a module
 *
 * @param name		the module to revoke
 * @result			true if successful, false on error
 */
API bool revokeModule(char *name);

/**
 * Unloads a module by force, i.e. first unloads its reverse dependencies recursively and then the module itself
 *
 * @param name		the module to force unload
 * @result			true if successful, false on error
 */
API bool forceUnloadModule(char *name);

/**
 * Adds a runtime dependency from one module to another. It isn't possible to add circular dependencies, and once a runtime dependency is set, it cannot be removed manually and remains effective until the source module is revoked
 *
 * @param source		the name of the module to add the runtime dependency to
 * @param target		the name of the target dependency module
 * @result				true if successful
 */
API bool addModuleRuntimeDependency(char *source, char *target);

/**
 * Checks if a module depends on another module
 *
 * @param source		the source module to check for a dependency
 * @param target		the target module to check for a dependency
 * @result				true if source depends on target (directly or indirectly)
 */
API bool checkModuleDependency(char *source, char *target);

/**
 * Attempts to fetch a function from a module by name.
 *
 * @param module_name           the name of the module to search in
 * @param function_name         the name of the function to search for
 * @result                      a pointer to the function if found, NULL otherwise
 */
API void *getLibraryFunctionByName(char *module_name, char *function_name);

#define MODULE_NAME_FUNC "module_name"
#define MODULE_AUTHOR_FUNC "module_author"
#define MODULE_DESCRIPTION_FUNC "module_description"
#define MODULE_VERSION_FUNC "module_version"
#define MODULE_BCVERSION_FUNC "module_bcversion"
#define MODULE_DEPENDS_FUNC "module_depends"

#define MODULE_INITIALIZER_FUNC "module_init"
#define MODULE_FINALIZER_FUNC "module_finalize"

#define MODULE_NAME(NAME) API char *module_name() { return NAME; }
#define MODULE_AUTHOR(AUTHOR) API char *module_author() { return AUTHOR; }
#define MODULE_DESCRIPTION(DESC) API char *module_description() { return DESC; }
#define MODULE_VERSION(MAJOR, MINOR, PATCH) static Version _module_version = {MAJOR, MINOR, PATCH, "SRC_REVISION"}; \
	API Version *module_version() { return &_module_version; }
#define MODULE_BCVERSION(MAJOR, MINOR, PATCH) static Version _module_bcversion = {MAJOR, MINOR, PATCH, "0"}; \
	API Version *module_bcversion() { return &_module_bcversion; }

#define MODULE_NODEPS static ModuleDependency _module_dependencies[] = {{NULL,{-1,-1,-1,"0"}}}; \
	API ModuleDependency *module_depends() { return _module_dependencies; }
#define MODULE_DEPENDS(...) static ModuleDependency _module_dependencies[] = {__VA_ARGS__, {NULL,{-1,-1,-1,"0"}}}; \
	API ModuleDependency *module_depends() { return _module_dependencies; }

#define MODULE_DEPENDENCY(NAME, MAJOR, MINOR, PATCH) {NAME, {MAJOR, MINOR, PATCH, "0"}}

#define MODULE_INIT API bool module_init()
#define MODULE_FINALIZE API void module_finalize()

#endif
