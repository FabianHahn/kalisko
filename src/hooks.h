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


#ifndef HOOKS_H
#define HOOKS_H

#include <glib.h>
#include <stdarg.h>
#include "types.h"

/**
 * Generic hook listener function pointer
 */
typedef void (HookListener)(const char *hook_name, void *custom_data, va_list args);

/**
 * Struct to represent a listener entry
 */
typedef struct
{
	/** The listener function pointer */
	HookListener *listener;
	/** Custom data to pass to the function when triggered */
	void *custom_data;
} HookListenerEntry;

/**
 * Struct to represent a stats entry
 */
typedef struct
{
	/** The name of the hook */
	char *hook_name;
	/** The number of listeners attached to this hook */
	int num_listeners;
} HookStatsEntry;

API void initHooks();
API void freeHooks();
API bool addHook(char *hook_name);
API bool delHook(char *hook_name);
API bool attachToHook(char *hook_name, HookListener *listener, void *custom_data);
API bool detachFromHook(char *hook_name, HookListener *listener, void *custom_data);
API int triggerHook(char *hook_name, ...);
API GList *getHookStats();
API void freeHookStats(GList *hook_stats);

// Wrapper macros for hooks
/**
 * Transforms the given listener name to a general valid form.
 * This is used for example in the listeners function head as the function name.
 *
 * @param LISTENER	name of the hook listener
 */
#define HOOK_LISTENER_NAME(LISTENER) _hook_listener_ ## LISTENER

/**
 * Transforms the given listener name to a valid function head.
 *
 * @param LISTENER	name of the hook listener. Must be unique in the scope of the sourcefile.
 */
#define HOOK_LISTENER(LISTENER) static void HOOK_LISTENER_NAME(LISTENER)(const char *hook_name, void *custom_data, va_list args)

/**
 * Returns an hook trigger argument.
 * Attention: You must use this in the same order as triggered with HOOK_TRIGGER
 * and you must also get the first argument if you want the second, and so on... .
 *
 * @see HOOK_TRIGGER
 * @param TYPE	the type of the next argument to return
 */
#define HOOK_ARG(TYPE) va_arg(args, TYPE)

#ifdef DLL_API_IMPORT

/**
 * @see HOOK_TRIGGER
 * @see HOOK_LISTENER
 * @see attachToHook
 * @param HOOK			the name of the hook
 * @param LISTENER		the hook listener to attach
 * @result				true if successful
 */
#define HOOK_ATTACH(HOOK, LISTENER) $$(bool, attachToHook)(#HOOK, &HOOK_LISTENER_NAME(LISTENER), NULL)

/**
 * @see HOOK_TRIGGER
 * @see HOOK_LISTENER
 * @see attachToHook
 * @param HOOK			the hook to which the listener will be attached
 * @param LISTENER		the listener to attach
 * @param CDATA			custom data to pass to the hook listeners when triggered
 * @result				true if successful
 */
#define HOOK_ATTACH_EX(HOOK, LISTENER, CDATA) $$(bool, attachToHook)(#HOOK, &HOOK_LISTENER_NAME(LISTENER), CDATA)

/**
 * @see HOOK_ATTACH
 * @see detachFromHook
 * @param HOOK			the hook from which the listener will be detached
 * @param LISTENER		the listener to detached
 * @result				true if successful
 */
#define HOOK_DETACH(HOOK, LISTENER) $$(bool, detachFromHook)(#HOOK, &HOOK_LISTENER_NAME(LISTENER), NULL)

/**
 * @see HOOK_ATTACH
 * @see detachFromHook
 * @param HOOK			the hook from which the listener will be detached
 * @param LISTENER		the listener to detached
 * @param CDATA			custom data which was passed to the hook listeners when triggered
 * @result				true if successful
 */
#define HOOK_DETACH_EX(HOOK, LISTENER, CDATA) $$(bool, detachFromHook)(#HOOK, &HOOK_LISTENER_NAME(LISTENER), CDATA)

/**
 * @see addHook
 * @param HOOK	the name of the hook
 * @result				true if successful
 */
#define HOOK_ADD(HOOK) $$(bool, addHook)(#HOOK)

/**
 * @see delHook
 * @param HOOK	the name of the hook
 * @result				true if successful
 */
#define HOOK_DEL(HOOK) $$(bool, delHook)(#HOOK)

/**
 * @see triggerHook
 * @param HOOK			the name of the hook
 * @param ...			the data to pass to the listeners
 * @result				the number of listeners notified, -1 if hook not found
 */
#define HOOK_TRIGGER(HOOK, ...) $$(int, triggerHook)(#HOOK, __VA_ARGS__)

#endif

#endif
