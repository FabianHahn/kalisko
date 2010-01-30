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

#include <assert.h>
#include <string.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "store.h"
#include "merge.h"
#include "clone.h"

static void mergeIntoArray(void *key_p, void *value_p, void *data_p);

/**
 * Imports the content of the import store into the target store
 *
 * @param target	the store to merge to
 * @param import	the store to import from
 * @result			true if successful
 */
API bool mergeStore(Store *target, Store *import)
{
	assert(target != NULL);
	assert(import != NULL);

	if(target->type != import->type) { // Cannot merge stores from different types
		return false;
	}

	switch(import->type) {
		case STORE_ARRAY:
			g_hash_table_foreach(import->content.array, &mergeIntoArray, target->content.array);
		break;
		case STORE_LIST:
			for(GList *iter = import->content.list->head; iter != NULL; iter = iter->next) {
				g_queue_push_tail(target->content.list, cloneStore(iter->data)); // Clone and insert
			}
		break;
		default:
			return false; // Cannot merge non-container store
		break;
	}

	return true;
}

/**
 * A GHFunc to merge a store array value into another store array
 *
 * @param key_p		the key of the store array node to merge
 * @param value_p	the value of the store array node to merge
 * @param data_p	the store array to merge into
 */
static void mergeIntoArray(void *key_p, void *value_p, void *data_p)
{
	char *key = key_p;
	Store *value = value_p;
	GHashTable *target = data_p;

	Store *candidate = g_hash_table_lookup(target, key);

	if(candidate == NULL) { // there is no node with that key in the target
		g_hash_table_insert(target, strdup(key), cloneStore(value)); // clone and insert
	} else {
		// try to merge the two
		if(!mergeStore(candidate, value)) { // if merging doesn't work, we need to replace our candidate
			g_hash_table_replace(target, strdup(key), cloneStore(value)); // clone and replace
		}
	}
}
