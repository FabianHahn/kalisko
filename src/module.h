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

API void initModules();
API void freeModules();
API char *getModuleSearchPath();
API void setModuleSearchPath(char *path);
API void resetModuleSearchPath();
API char *getModuleAuthor(const char *name);
API char *getModuleDescription(const char *name);
API Version *getModuleVersion(const char *name);
API Version *getModuleBcVersion(const char *name);
API int getModuleReferenceCount(const char *name);
API GList *getModuleDependencies(const char *name);
API GList *getModuleReverseDependencies(const char *name);
API GList *getActiveModules();
API bool isModuleLoaded(const char *name);
API bool isModuleRequested(const char *name);
API bool requestModule(char *name);
API bool revokeModule(char *name);
API bool forceUnloadModule(char *name);
API bool addModuleRuntimeDependency(char *source, char *target);
API bool checkModuleDependency(char *source, char *target);

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
#define MODULE_VERSION(MAJOR, MINOR, PATCH) static Version _module_version = {MAJOR, MINOR, PATCH, SRC_REVISION}; \
	API Version *module_version() { return &_module_version; }
#define MODULE_BCVERSION(MAJOR, MINOR, PATCH) static Version _module_bcversion = {MAJOR, MINOR, PATCH, 0}; \
	API Version *module_bcversion() { return &_module_bcversion; }

#define MODULE_NODEPS static ModuleDependency _module_dependencies[] = {{NULL,{-1,-1,-1,-1}}}; \
	API ModuleDependency *module_depends() { return _module_dependencies; }
#define MODULE_DEPENDS(...) static ModuleDependency _module_dependencies[] = {__VA_ARGS__, {NULL,{-1,-1,-1,-1}}}; \
	API ModuleDependency *module_depends() { return _module_dependencies; }

#define MODULE_DEPENDENCY(NAME, MAJOR, MINOR, PATCH) {NAME, {MAJOR, MINOR, PATCH, 0}}

#define MODULE_INIT API bool module_init()
#define MODULE_FINALIZE API void module_finalize()

#endif
