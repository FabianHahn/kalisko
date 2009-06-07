/**
 * @file hooks.h
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


#ifndef HOOKS_H
#define HOOKS_H

#include <glib.h>
#include <stdarg.h>
#include "types.h"

// Generic hook listener function pointer
typedef void (HookListener)(const char *hook_name, void *custom_data, va_list args);

// Listener entry struct
typedef struct
{
	HookListener *listener; // The listener function pointer
	void *custom_data; // Custom data to pass to the function when triggered
} HookListenerEntry;

API void initHooks();
API void freeHooks();
API bool addHook(const char *hook_name);
API bool delHook(const char *hook_name);
API bool attachToHook(const char *hook_name, HookListener *listener, void *custom_data);
API bool detachFromHook(const char *hook_name, HookListener *listener, void *custom_data);
API int triggerHook(const char *hook_name, ...);

// Wrapper macros for hooks
#define HOOK_LISTENER_NAME(LISTENER) _hook_listener_ ## LISTENER
#define HOOK_LISTENER(LISTENER) static void HOOK_LISTENER_NAME(LISTENER)(const char *hook_name, void *custom_data, va_list args)
#define HOOK_ARG(TYPE) va_arg(args, TYPE)
#define HOOK_ATTACH(HOOK, LISTENER) attachToHook(#HOOK, &HOOK_LISTENER_NAME(LISTENER), NULL)
#define HOOK_ATTACH_EX(HOOK, LISTENER, CDATA) attachToHook(#HOOK, &HOOK_LISTENER_NAME(LISTENER), CDATA)
#define HOOK_DETACH(HOOK, LISTENER) detachFromHook(#HOOK, &HOOK_LISTENER_NAME(LISTENER), NULL)
#define HOOK_DETACH_EX(HOOK, LISTENER, CDATA) detachFromHook(#HOOK, &HOOK_LISTENER_NAME(LISTENER), CDATA)
#define HOOK_ADD(HOOK) addHook(#HOOK)
#define HOOK_DEL(HOOK) delHook(#HOOK)
#define HOOK_TRIGGER(HOOK, ...) triggerHook(#HOOK, __VA_ARGS__)

#endif
