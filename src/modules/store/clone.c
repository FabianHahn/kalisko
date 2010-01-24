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
#include <assert.h>
#include <string.h>

#include "dll.h"
#include "store.h"
#include "api.h"

static void cloneStoreArrayNode(void *key_p, void *value_p, void *data_p);

/**
 * Clones a store and returns a second, identical one
 *
 * @param source	the store to copy from
 * @result			an identical store
 */
API Store *cloneStore(Store *source)
{
	assert(source != NULL);

	Store *clone;

	switch(source->type) {
		case STORE_ARRAY:
			clone = createStoreArrayValue(NULL);
			g_hash_table_foreach(source->content.array, &cloneStoreArrayNode, clone->content.array);
			return clone;
		break;
		case STORE_LIST:
			clone = createStoreListValue(NULL);
			for(GList *iter = source->content.list->head; iter != NULL; iter = iter->next) {
				g_queue_push_tail(clone->content.list, cloneStore(iter->data));
			}
			return clone;
		break;
		case STORE_STRING:
			return createStoreStringValue(strdup(source->content.string));
		break;
		case STORE_INTEGER:
			return createStoreIntegerValue(source->content.integer);
		break;
		case STORE_FLOAT_NUMBER:
			return createStoreFloatNumberValue(source->content.float_number);
		break;
	}

	return NULL;
}

/**
 * A GHFunc to clone a store array node
 *
 * @param key_p		a pointer to the array key
 * @param value_p	a pointer to the store value
 * @param data_p	the hash table to add the cloned store value to
 */
static void cloneStoreArrayNode(void *key_p, void *value_p, void *data_p)
{
	char *key = key_p;
	Store *value = value_p;
	GHashTable *target = data_p;

	g_hash_table_insert(target, strdup(key), cloneStore(value));
}