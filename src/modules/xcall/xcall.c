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
#include "modules/store/store.h"
#include "api.h"
#include "xcall.h"

MODULE_NAME("xcall");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The xcall module provides a powerful interface for cross function calls between different languages");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 3, 0));

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
API bool delXCall(const char *name)
{
	if(g_hash_table_lookup(functions, name) != NULL) { // A function with that name doesn't exist
		return false;
	}

	// Remove the function
	g_hash_table_remove(functions, name);

	return true;
}

/**
 * Invokes an xcall
 *
 * @param xcall		a string store containing the xcall
 * @result			a string store containing the result of the xcall
 */
API GString *invokeXCall(const char *xcall)
{
	Store *xcallstore;
	Store *retstore = $(Store *, store, createStore(NULL));

	if((xcallstore = $(Store *, store, parseStoreString(xcall))) == NULL)
	{

	}

	Store *value;

	//if((value = $(Store *, store, getstore)))
}

