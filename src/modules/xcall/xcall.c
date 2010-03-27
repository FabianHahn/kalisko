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
#include "api.h"
#include "xcall.h"

MODULE_NAME("xcall");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The xcall module provides a powerful interface for cross function calls between different languages");
MODULE_VERSION(0, 2, 3);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0));

static GHashTable *functions;

MODULE_INIT
{
	functions = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(functions);
}

/**
 * Adds a new XCall function
 *
 * @param name		the name of the hook
 * @param func		the xcall to add
 * @result			true if successful, false if the xcall already exists
 */
API bool addXCallFunction(const char *name, XCallFunction *func)
{
	if(g_hash_table_lookup(functions, name) != NULL) { // A xcall with that name already exists
		return false;
	}

	// Insert the xcall
	g_hash_table_insert(functions, strdup(name), func);

	return true;
}

/**
 * Deletes an existing xcall
 *
 * @param name		the name of the xcall
 * @result			true if successful, if the xcall was not found
 */
API bool delXCallFunction(const char *name)
{
	if(g_hash_table_lookup(functions, name) == NULL) { // A function with that name doesn't exist
		return false;
	}

	// Remove the function
	g_hash_table_remove(functions, name);

	return true;
}

/**
 * Invokes an xcall
 *
 * @param xcall		a store containing the xcall
 * @result			a store containing the result of the xcall, must be freed by the caller with freeStore
 */
API Store *invokeXCall(Store *xcall)
{
	Store *retstore = NULL;
	Store *metaret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(metaret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	do { // dummy do-while to prevent mass if-then-else branching
		Store *meta;

		if((meta = $(Store *, store, getStorePath)(xcall, "xcall")) == 0 || meta->type != STORE_ARRAY) {
			GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
			LOG_ERROR("Failed to read XCall meta array: %s", xcallstr->str);
			g_string_free(xcallstr, true);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read XCall meta array"));
			break;
		}

		Store *params = $(Store *, store, cloneStore)(xcall);
		$(bool, store, deleteStorePath)(params, "xcall"); // remove meta data from params in order to reuse the rest
		$(bool, store, setStorePath)(metaret, "xcall/params", params);

		Store *func;

		if((func = $(Store *, store, getStorePath)(meta, "function")) == NULL || func->type != STORE_STRING) {
			GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
			LOG_ERROR("Failed to read XCall function name: %s", xcallstr->str);
			g_string_free(xcallstr, true);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read XCall function name"));
			break;
		}

		char *funcname = func->content.string;
		$(bool, store, setStorePath)(metaret, "xcall/function", $(Store *, store, createStoreStringValue)(funcname));

		XCallFunction *function;

		if((function = g_hash_table_lookup(functions, funcname)) == NULL) {
			GString *xcallstr = $(GString *, store, writeStoreGString)(xcall);
			LOG_ERROR("Requested XCall functon '%s' not found: %s", funcname, xcallstr->str);
			g_string_free(xcallstr, true);
			GString *err = g_string_new("");
			g_string_append_printf(err, "Requested XCall function '%s' not found", funcname);
			$(bool, store, setStorePath)(metaret, "xcall/error", $(Store *, store, createStoreStringValue)(err->str));
			g_string_free(err, true);
			break;
		}

		retstore = function(xcall);

		if(retstore == NULL) { // Invalid returned store
			LOG_ERROR("Requested XCall function '%s' returned invalid store", funcname);
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

/**
 * Invokes an xcall by a string store
 *
 * @param xcallstr	a store string containing the xcall
 * @result			a store containing the result of the xcall, must be freed by the caller with freeStore
 */
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
	}

	return ret;
}

