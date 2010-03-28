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

#include <stdarg.h>
#include <glib.h>
#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "module.h"
#include "modules/xcall/xcall.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "api.h"
#include "version2store.h"

MODULE_NAME("xcall_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module which offers an XCall API to the Kalisko Core");
MODULE_VERSION(0, 3, 3);
MODULE_BCVERSION(0, 3, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

static Store *xcall_attachLog(Store *xcall);
static Store *xcall_detachLog(Store *xcall);
static Store *xcall_getModuleAuthor(Store *xcall);
static Store *xcall_getModuleDescription(Store *xcall);
static Store *xcall_getModuleVersion(Store *xcall);
static Store *xcall_getModuleBcVersion(Store *xcall);
static Store *xcall_getModuleReferenceCount(Store *xcall);
static Store *xcall_getActiveModules(Store *xcall);
static Store *xcall_isModuleLoaded(Store *xcall);
static Store *xcall_requestModule(Store *xcall);
static Store *xcall_revokeModule(Store *xcall);
static Store *xcall_logError(Store *xcall);
static Store *xcall_logWarning(Store *xcall);
static Store *xcall_logInfo(Store *xcall);
static Store *xcall_logDebug(Store *xcall);
static bool attachLogListener(char *listener);
static bool detachLogListener(char *listener);
static void freeLogListenerEntry(void *listener_p);

HOOK_LISTENER(xcall_log);

GHashTable *logListeners;
static bool logExecuting;

MODULE_INIT
{
	logExecuting = false;
	logListeners = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeLogListenerEntry);

	$(bool, xcall, addXCallFunction)("attachLog", &xcall_attachLog);
	$(bool, xcall, addXCallFunction)("detachLog", &xcall_detachLog);
	$(bool, xcall, addXCallFunction)("getModuleAuthor", &xcall_getModuleAuthor);
	$(bool, xcall, addXCallFunction)("getModuleDescription", &xcall_getModuleDescription);
	$(bool, xcall, addXCallFunction)("getModuleVersion", &xcall_getModuleVersion);
	$(bool, xcall, addXCallFunction)("getModuleBcVersion", &xcall_getModuleBcVersion);
	$(bool, xcall, addXCallFunction)("getModuleReferenceCount", &xcall_getModuleReferenceCount);
	$(bool, xcall, addXCallFunction)("getActiveModules", &xcall_getActiveModules);
	$(bool, xcall, addXCallFunction)("isModuleLoaded", &xcall_isModuleLoaded);
	$(bool, xcall, addXCallFunction)("requestModule", &xcall_requestModule);
	$(bool, xcall, addXCallFunction)("revokeModule", &xcall_revokeModule);
	$(bool, xcall, addXCallFunction)("logError", &xcall_logError);
	$(bool, xcall, addXCallFunction)("logWarning", &xcall_logWarning);
	$(bool, xcall, addXCallFunction)("logInfo", &xcall_logInfo);
	$(bool, xcall, addXCallFunction)("logDebug", &xcall_logDebug);

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("attachLog");
	$(bool, xcall, delXCallFunction)("detachLog");
	$(bool, xcall, delXCallFunction)("getModuleAuthor");
	$(bool, xcall, delXCallFunction)("getModuleDescription");
	$(bool, xcall, delXCallFunction)("getModuleVersion");
	$(bool, xcall, delXCallFunction)("getModuleBcVersion");
	$(bool, xcall, delXCallFunction)("getModuleReferenceCount");
	$(bool, xcall, delXCallFunction)("getActiveModule");
	$(bool, xcall, delXCallFunction)("isModuleLoaded");
	$(bool, xcall, delXCallFunction)("requestModule");
	$(bool, xcall, delXCallFunction)("revokeModule");
	$(bool, xcall, delXCallFunction)("logError");
	$(bool, xcall, delXCallFunction)("logWarning");
	$(bool, xcall, delXCallFunction)("logInfo");
	$(bool, xcall, delXCallFunction)("logDebug");

	g_hash_table_destroy(logListeners);
}

/**
 * XCallFunction to attach an XCall function listener to a log hook
 * XCall parameters:
 *  * string listener	the xcall function to attach to the hook
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_attachLog(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *listener = $(Store *, store, getStorePath)(xcall, "listener");

	if(listener == NULL || listener->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *function = listener->content.string;
		bool attached = attachLogListener(function);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(attached));

		if(attached) {
			LOG_INFO("Attached XCall function '%s' to log hook", function);
		}
	}

	return retstore;
}

/**
 * XCallFunction to detach an XCall function listener from the log hook
 * XCall parameters:
 *  * string listener	the xcall function to attach to the hook
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_detachLog(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *listener = $(Store *, store, getStorePath)(xcall, "listener");

	if(listener == NULL || listener->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *function = listener->content.string;
		bool attached = detachLogListener(function);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(attached));

		if(attached) {
			LOG_INFO("Detached XCall function '%s' from log hook", function);
		}
	}

	return retstore;
}

HOOK_LISTENER(xcall_log)
{
	if(logExecuting) {
		return; // Prevent infinite loop
	}

	logExecuting = true;

	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);
	char *listener = custom_data;

	Store *xcall = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(xcall, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
	$(bool, store, setStorePath)(xcall, "xcall/function", $(Store *, store, createStoreStringValue)(listener));

	switch(type) {
		case LOG_TYPE_DEBUG:
			$(bool, store, setStorePath)(xcall, "log_type", $(Store *, store, createStoreStringValue)("debug"));
		break;
		case LOG_TYPE_INFO:
			$(bool, store, setStorePath)(xcall, "log_type", $(Store *, store, createStoreStringValue)("info"));
		break;
		case LOG_TYPE_WARNING:
			$(bool, store, setStorePath)(xcall, "log_type", $(Store *, store, createStoreStringValue)("warning"));
		break;
		case LOG_TYPE_ERROR:
			$(bool, store, setStorePath)(xcall, "log_type", $(Store *, store, createStoreStringValue)("error"));
		break;
	}

	$(bool, store, setStorePath)(xcall, "message", $(Store *, store, createStoreStringValue)(message));

	Store *ret = $(Store *, xcall, invokeXCall)(xcall);
	$(void, store, freeStore)(xcall);

	Store *error = $(Store *, store, getStorePath)(ret, "xcall/error");

	if(error != NULL && error->type == STORE_STRING) { // XCall error
		detachLogListener(listener); // detach the listener
		LOG_ERROR("Attached log XCall function '%s' failed: %s", listener, error->content.string);
	}

	$(void, store, freeStore)(ret);

	logExecuting = false;
}

/**
 * Attaches an XCall function listener to the log hook
 * @param listener		the listener to be detached
 * @result				true if successful
 */
static bool attachLogListener(char *listener)
{
	if(g_hash_table_lookup(logListeners, listener) != NULL) { // Listener with that name already exists
		return false;
	}

	char *identifier = strdup(listener);

	g_hash_table_insert(logListeners, identifier, identifier);
	return HOOK_ATTACH_EX(log, xcall_log, identifier);
}

/**
 * Detaches an attached XCall function listener to the log hook
 * @param listener		the listener to be detached
 * @result				true if successful
 */
static bool detachLogListener(char *listener)
{
	return g_hash_table_remove(logListeners, listener);
}

/**
 * GDestroyNotify to free an attached XCall function listener to the log hook
 * @param listener_p		a pointer to the listener to be freed
 */
static void freeLogListenerEntry(void *listener_p)
{
	char *listener = listener_p;

	HOOK_DETACH_EX(log, xcall_log, listener_p);

	free(listener);
}

/**
 * XCallFunction to fetch a module's author
 * XCall parameters:
 *  * string module		the module to check
 * XCall result:
 * 	* string author		the author of the module if it's at least loading
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getModuleAuthor(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		char *author = $$(char *, getModuleAuthor)(modname);

		if(author != NULL) {
			$(bool, store, setStorePath)(retstore, "author", $(Store *, store, createStoreStringValue)(author));
		}
	}

	return retstore;
}

/**
 * XCallFunction to fetch a module's description
 * XCall parameters:
 *  * string module			the module to check
 * XCall result:
 * 	* string description	the description of the module if it's at least loading
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getModuleDescription(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		char *description = $$(char *, getModuleDescription)(modname);

		if(description != NULL) {
			$(bool, store, setStorePath)(retstore, "description", $(Store *, store, createStoreStringValue)(description));
		}
	}

	return retstore;
}

/**
 * XCallFunction to fetch a module's version
 * XCall parameters:
 *  * string module		the module to check
 * XCall result:
 *  * array version				exists if the module is at least loading
 * 	* int version/major			the major version of the module
 * 	* int version/minor			the minor version of the module
 * 	* int version/patch			the patch version of the module
 * 	* int version/revision		the revision version of the module
 *  * string version/string		the whole version as string
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getModuleVersion(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		Version *version = $$(Version *, getModuleVersion)(modname);

		if(version != NULL) {
			$(bool, store, setStorePath)(retstore, "version", version2Store(version));
		}
	}

	return retstore;
}

/**
 * XCallFunction to fetch a module's backwards compatible version
 * XCall parameters:
 *  * string module		the module to check
 * XCall result:
 *  * array bcversion				exists if the module is at least loading
 * 	* int bcversion/major			the major backwards compatible of the module
 * 	* int bcversion/minor			the minor backwards compatible of the module
 * 	* int bcversion/patch			the patch backwards compatible of the module
 * 	* int bcversion/revision		the revision backwards compatible of the module
 *  * string bcversion/string		the whole backwards compatible version as string
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getModuleBcVersion(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		Version *bcversion = $$(Version *, getModuleBcVersion)(modname);

		if(bcversion != NULL) {
			$(bool, store, setStorePath)(retstore, "bcversion", version2Store(bcversion));
		}
	}

	return retstore;
}

/**
 * XCallFunction to fetch a module's reference count
 * XCall parameters:
 *  * string module			the module to check
 * XCall result:
 * 	* int reference_count	the current reference count of the module if it's at least loaded
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getModuleReferenceCount(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		int rc = $$(int, getModuleReferenceCount)(modname);

		if(rc >= 0) {
			$(bool, store, setStorePath)(retstore, "reference_count", $(Store *, store, createStoreIntegerValue)(rc));
		}
	}

	return retstore;
}

/**
 * XCallFunction to fetch all active modules. A module is considered active if it's either already loaded or currently loading
 * XCall result:
 * 	* list modules 			a string list of all active modules
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getActiveModules(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	GList *modules = $$(GList *, getActiveModules)();
	Store *modulesStore = $(Store *, store, createStoreListValue)(NULL);

	for(GList *iter = modules; iter != NULL; iter = iter->next) {
		g_queue_push_tail(modulesStore->content.list, $(Store *, store, createStoreStringValue)(iter->data));
	}

	$(bool, store, setStorePath)(retstore, "modules", modulesStore);
	g_list_free(modules);

	return retstore;
}


/**
 * XCallFunction to check if a module is loaded
 * XCall parameters:
 *  * string xcall			the module to check
 * XCall result:
 * 	* int loaded			positive if the module is loaded
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_isModuleLoaded(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, isModuleLoaded)(modname);
		$(bool, store, setStorePath)(retstore, "loaded", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	return retstore;
}


/**
 * XCallFunction to request a module
 * XCall parameters:
 *  * string module		the module to request
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_requestModule(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, requestModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	return retstore;
}

/**
 * XCallFunction to revoke a module
 * XCall parameters:
 *  * string module		the module to revoke
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_revokeModule(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcall, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, revokeModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	return retstore;
}

/**
 * XCallFunction to log an error message
 * XCall parameters:
 *  * string message		the message to log
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_logError(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *message = $(Store *, store, getStorePath)(xcall, "message");

	if(message == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));
	} else {
		LOG_ERROR("%s", message->content.string);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(1));
	}

	return retstore;
}

/**
 * XCallFunction to log a warning message
 * XCall parameters:
 *  * string message		the message to log
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_logWarning(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *message = $(Store *, store, getStorePath)(xcall, "message");

	if(message == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));
	} else {
		LOG_WARNING("%s", message->content.string);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(1));
	}

	return retstore;
}

/**
 * XCallFunction to log an info message
 * XCall parameters:
 *  * string message		the message to log
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_logInfo(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *message = $(Store *, store, getStorePath)(xcall, "message");

	if(message == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));
	} else {
		LOG_INFO("%s", message->content.string);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(1));
	}

	return retstore;
}

/**
 * XCallFunction to log a debug message
 * XCall parameters:
 *  * string message		the message to log
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_logDebug(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *message = $(Store *, store, getStorePath)(xcall, "message");

	if(message == NULL || message->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'message'"));
	} else {
		LOG_DEBUG(message->content.string, NULL);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(1));
	}

	return retstore;
}
