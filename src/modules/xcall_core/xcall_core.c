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


MODULE_NAME("xcall_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module which offers an XCall API to the Kalisko Core");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 6, 0));

static GString *xcall_attachLog(const char *xcall);
static GString *xcall_detachLog(const char *xcall);
static GString *xcall_requestModule(const char *xcall);
static GString *xcall_revokeModule(const char *xcall);
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
	$(bool, xcall, addXCallFunction)("requestModule", &xcall_requestModule);
	$(bool, xcall, addXCallFunction)("revokeModule", &xcall_revokeModule);

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("attachLog");
	$(bool, xcall, delXCallFunction)("detachLog");
	$(bool, xcall, delXCallFunction)("requestModule");
	$(bool, xcall, delXCallFunction)("revokeModule");

	g_hash_table_destroy(logListeners);
}

/**
 * XCallFunction to attach an XCall function listener to a log hook
 * XCall parameters:
 *  * string listener	the xcall function to attach to the hook
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_attachLog(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *listener = $(Store *, store, getStorePath)(xcs, "listener");

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

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
}

/**
 * XCallFunction to detach an XCall function listener from the log hook
 * XCall parameters:
 *  * string listener	the xcall function to attach to the hook
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_detachLog(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *listener = $(Store *, store, getStorePath)(xcs, "listener");

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

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
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

	GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
	$(void, store, freeStore)(xcall);

	GString *ret = $(GString *, xcall, invokeXCall)(xcallstr->str);
	g_string_free(xcallstr, true);

	Store *xcs = $(Store *, store, parseStoreString)(ret->str);
	g_string_free(ret, true);

	Store *error = $(Store *, xcall, getStorePath)(xcs, "xcall/error");

	if(error->type == STORE_STRING) { // XCall error
		detachLogListener(listener); // detach the listener
		LOG_ERROR("Attached log XCall function '%s' failed: %s", listener, error->content.string);
	}

	$(void, store, freeStore)(xcall);

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
	char *identifier;

	if((identifier = g_hash_table_lookup(logListeners, listener)) == NULL) {
		return false;
	}

	freeLogListenerEntry(identifier);

	return true;
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
 * XCallFunction to request a module
 * XCall parameters:
 *  * string module		the module to request
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_requestModule(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcs, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, requestModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
}

/**
 * XCallFunction to revoke a module
 * XCall parameters:
 *  * string module		the module to revoke
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_revokeModule(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcs, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, revokeModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
}
