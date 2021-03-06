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
#include <string.h>

#include "dll.h"
#include "log.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/store/clone.h"
#include "modules/store/write.h"
#include "modules/store/merge.h"
#define API
#include "xcall.h"

MODULE_NAME("xcall");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The xcall module provides a powerful interface for cross function calls between different languages");
MODULE_VERSION(0, 2, 8);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0));

static Store *xcall_getXCallFunctions(Store *xcall);

static GHashTable *functions;

MODULE_INIT
{
	functions = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	addXCallFunction("getXCallFunctions", &xcall_getXCallFunctions);

	return true;
}

MODULE_FINALIZE
{
	delXCallFunction("getXCallFunctions");
	g_hash_table_destroy(functions);
}

API bool addXCallFunction(const char *name, XCallFunction *func)
{
	if(g_hash_table_lookup(functions, name) != NULL) { // A xcall with that name already exists
		return false;
	}

	// Insert the xcall
	g_hash_table_insert(functions, strdup(name), func);

	return true;
}

API bool delXCallFunction(const char *name)
{
	if(g_hash_table_lookup(functions, name) == NULL) { // A function with that name doesn't exist
		return false;
	}

	// Remove the function
	g_hash_table_remove(functions, name);

	return true;
}

API bool existsXCallFunction(const char *name)
{
	if(g_hash_table_lookup(functions, name) != NULL) { // A xcall with that name already exists
		return true;
	}

	return false;
}

API Store *invokeXCall(Store *xcall)
{
	Store *retstore = NULL;
	Store *metaret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(metaret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	do { // dummy do-while to prevent mass if-then-else branching
		Store *params = $(Store *, store, cloneStore)(xcall);
		$(bool, store, deleteStorePath)(params, "xcall"); // remove meta data from params in order to reuse the rest
		$(bool, store, setStorePath)(metaret, "xcall/params", params);

		Store *func;

		// First try to read function name directly
		if((func = $(Store *, store, getStorePath)(xcall, "xcall")) != NULL && func->type == STORE_STRING) { // it is set directly
			Store *metafunction = $(Store *, store, createStoreStringValue)(func->content.string);
			$(bool, store, deleteStorePath)(xcall, "xcall"); // remove old xcall value
			$(bool, store, setStorePath)(xcall, "xcall", $(Store *, store, createStore)()); // create meta array
			$(bool, store, setStorePath)(xcall, "xcall/function", metafunction); // copy metafunction into meta array
		}

		// Now try to read function name inside xcall meta array
		if((func = $(Store *, store, getStorePath)(xcall, "xcall/function")) == NULL || func->type != STORE_STRING) {
			GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
			logError("Failed to read XCall function name: %s", xcallstr->str);
			g_string_free(xcallstr, true);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read XCall function name"));
			break;
		}

		char *funcname = func->content.string;
		$(bool, store, setStorePath)(metaret, "xcall/function", $(Store *, store, createStoreStringValue)(funcname));

		XCallFunction *function;

		if((function = g_hash_table_lookup(functions, funcname)) == NULL) {
			GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
			logError("Requested XCall functon '%s' not found: %s", funcname, xcallstr->str);
			g_string_free(xcallstr, true);
			GString *err = g_string_new("");
			g_string_append_printf(err, "Requested XCall function '%s' not found", funcname);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
			g_string_free(err, true);
			break;
		}

		retstore = function(xcall);

		if(retstore == NULL) { // Invalid returned store
			logError("Requested XCall function '%s' returned invalid store", funcname);
			GString *err = g_string_new("");
			g_string_append_printf(err, "Requested XCall function '%s' returned invalid store", funcname);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
			g_string_free(err, true);
			break;
		}

		$(bool, store, mergeStore)(retstore, metaret); // merge metadata into returned store
		$(void, store, freeStore)(metaret); // since we won't use metaret anymore, we can safely delete it
	} while(false);

	if(retstore == NULL) { // if there is no returned store, simply return the metadata
		retstore = metaret;
	}

	return retstore;
}

API Store *invokeXCallByString(const char *xcallstr)
{
	Store *ret;
	Store *xcall = $(Store *, store, parseStoreString)(xcallstr);

	if(xcall == NULL) {
		ret = $(Store *, store, createStore)();
		GString *err = g_string_new("");
		g_string_append_printf(err, "Failed to parse XCall store string: %s", xcallstr);
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
		g_string_free(err, true);
	} else {
		ret = invokeXCall(xcall);
		$(void, store, freeStore)(xcall);
	}

	return ret;
}

/**
 * XCallFunction to fetch all available XCall functions
 * XCall result:
 * 	* list functions		a string list of all available xcall functions
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_getXCallFunctions(Store *xcall)
{
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	GList *funcs = g_hash_table_get_keys(functions);
	Store *functionsStore = $(Store *, store, createStoreListValue)(NULL);

	for(GList *iter = funcs; iter != NULL; iter = iter->next) {
		g_queue_push_tail(functionsStore->content.list, $(Store *, store, createStoreStringValue)(iter->data));
	}

	$(bool, store, setStorePath)(retstore, "functions", functionsStore);
	g_list_free(funcs);

	return retstore;
}

