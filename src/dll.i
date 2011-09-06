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


#ifdef API
#undef API
#endif

#define $(TYPE, MODULE, FUNC) FUNC
#define $$(TYPE, FUNC) FUNC

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <windows.h>
#define GET_MODULE_PARAM(MODULE) (strlen(#MODULE) ? "kalisko_"#MODULE : NULL)
#else
#include <dlfcn.h>
#ifdef __cplusplus
extern "C" {
#endif
void *getModuleHandle(const char *name);
#ifdef __cplusplus
}
#endif
#define GET_MODULE_PARAM(MODULE) (strlen(#MODULE) ? #MODULE : NULL)
#endif

/**
 * Returns an API function of a module. This can be used without actually linking to the specified module's shared library.
 * Note that this only works for functions declared with "API" in their respective interface file.
 *
 * @param moduleName		the module from which to fetch the API function
 * @param functionName		the name of the function to fetch
 * @result					a function pointer to the API function
 */
static inline void *getApiFunction(const char *moduleName, const char *functionName)
{
#ifdef WIN32
	HMODULE handle = GetModuleHandle(moduleName);
	assert(handle != NULL);
	return (void *) GetProcAddress(handle, functionName);
#else
	void *handle = getModuleHandle(moduleName);
	assert(handle != NULL);
	return dlsym(handle, functionName);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

#include "module.h"
#include "log.h"
#include "memory_alloc.h"
#include "timer.h"
#include "types.h"
#include "util.h"
#include "version.h"

#ifdef __cplusplus
}
#endif

