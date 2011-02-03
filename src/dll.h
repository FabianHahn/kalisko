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

#ifndef DLL_API_IMPORT
#define DLL_API_IMPORT
#endif

#define $$(TYPE, FUNC) $(TYPE,, FUNC)

#ifdef WIN32
#include <stdio.h>
#include "windows.h"
#ifdef __cplusplus
#define $(TYPE, MODULE, FUNC) ((TYPE (*)(...)) GetProcAddress(GET_LIBRARY_HANDLE(MODULE), #FUNC))
#else
#define $(TYPE, MODULE, FUNC) ((TYPE (*)()) GetProcAddress(GET_LIBRARY_HANDLE(MODULE), #FUNC))
#endif
#define GET_LIBRARY_HANDLE(MODULE) GetModuleHandle(GET_MODULE_PARAM(MODULE))
#define GET_MODULE_PARAM(MODULE) (strlen(#MODULE) ? "kalisko_"#MODULE : NULL)
#define API __declspec(dllimport)
#else
#define $(TYPE, MODULE, FUNC) FUNC
#define API
#endif

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

