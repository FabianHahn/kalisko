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
#include "types.h"
#include "memory_alloc.h"
#include "util.h"

#define API
#include "store.h"

MODULE_NAME("store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The store module provides a recursive key-value data type that can be easily converted back and forth from a string and to its abstract memory representation");
MODULE_VERSION(0, 8, 0);
MODULE_BCVERSION(0, 5, 3);
MODULE_NODEPS;

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

API Store *createStore()
{
	return createStoreArrayValue(NULL);
}

API void freeStore(void *store)
{
	Store *value = store;

	switch(value->type) {
		case STORE_STRING:
			free(value->content.string);
		break;
		case STORE_LIST:
			for(GList *iter = value->content.list->head; iter != NULL; iter = iter->next) {
				freeStore(iter->data);
			}

			g_queue_free(value->content.list);
		break;
		case STORE_ARRAY:
			g_hash_table_destroy(value->content.array);
		break;
		default:
			// No need to free ints or doubles
		break;
	}

	free(value);
}


API void *getStoreValueContent(Store *value)
{
	switch(value->type) {
		case STORE_STRING:
			return value->content.string;
		break;
		case STORE_LIST:
			return value->content.list;
		break;
		case STORE_ARRAY:
			return value->content.array;
		break;
		default:
			return &value->content;
		break;
	}
}

API GString *escapeStoreString(char *string)
{
	GString *escaped = g_string_new("");

	for(char *iter = string; *iter != '\0'; iter++) {
		if(*iter == '"' || *iter == '\\') {
			g_string_append_printf(escaped, "\\%c", *iter); // escape the character
		} else {
			g_string_append_c(escaped, *iter);
		}
	}

	return escaped;
}

API Store *createStoreStringValue(const char *string)
{
	Store *value = ALLOCATE_OBJECT(Store);
	value->type = STORE_STRING;
	value->content.string = strdup(string);

	return value;
}

API Store *createStoreIntegerValue(int integer)
{
	Store *value = ALLOCATE_OBJECT(Store);
	value->type = STORE_INTEGER;
	value->content.integer = integer;

	return value;
}

API Store *createStoreFloatNumberValue(double float_number)
{
	Store *value = ALLOCATE_OBJECT(Store);
	value->type = STORE_FLOAT_NUMBER;
	value->content.float_number = float_number;

	return value;
}

API Store *createStoreListValue(GQueue *list)
{
	Store *value = ALLOCATE_OBJECT(Store);
	value->type = STORE_LIST;

	if(list != NULL) {
		value->content.list = list;
	} else {
		value->content.list = g_queue_new();
	}

	return value;
}

API Store *createStoreArrayValue(GHashTable *array)
{
	Store *value = ALLOCATE_OBJECT(Store);
	value->type = STORE_ARRAY;

	if(array != NULL) {
		value->content.array = array;
	} else {
		value->content.array = createStoreNodes();
	}

	return value;
}

API GHashTable *createStoreNodes()
{
	return g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeStore);
}
