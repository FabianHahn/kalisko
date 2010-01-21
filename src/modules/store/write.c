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
#include <stdio.h>

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"
#include "log.h"

#include "api.h"
#include "store.h"
#include "write.h"

/**
 * Convenience macro to write to store dumps
 *
 * @param FORMAT		printf-like format string
 */
#define DUMP(FORMAT, ...) context->writer(context->store, FORMAT, ##__VA_ARGS__)

static void storeFileWrite(Store *store, char *format, ...);
static void storeGStringWrite(Store *store, char *format, ...);
static void dumpStore(Store *store, StoreWriter *writer);
static void dumpStoreNode(void *key_p, void *value_p, void *data);
static void dumpStoreNodeValue(StoreNodeValue *value, StoreDumpContext *context);

/**
 * Writes a store from memory to a file
 *
 * @param filename		the filename to write to
 * @param store		the store to write
 */
API void writeStoreFile(char *filename, Store *store)
{
	store->resource = fopen(filename, "w");
	dumpStore(store, &storeFileWrite);
	fclose(store->resource);
}

/**
 * Writes a store from memory to a GString
 *
 * @param store		the store to write
 * @result				the written store string, must be freed with g_string_free
 */
API GString *writeStoreGString(Store *store)
{
	store->resource = g_string_new("");
	dumpStore(store, &storeGStringWrite);
	return store->resource;
}


/**
 * A StoreWriter function for files
 *
 * @param store		the store to to write to
 * @param format		printf-like string to write
 */
static void storeFileWrite(Store *store, char *format, ...)
{
	va_list va;

	va_start(va, format);

	vfprintf(store->resource, format, va);
}

/**
 * A StoreWriter function for GStrings
 *
 * @param store		the store to to write to
 * @param format		printf-like string to write
 */
static void storeGStringWrite(Store *store, char *format, ...)
{
	va_list va;

	va_start(va, format);

	g_string_append_vprintf(store->resource, format, va);
}

/**
 * Dumps a parsed store from memory to a writer
 *
 * @param store		the store to dump
 * @param writer		the writer where the store should be dumped to
 */
static void dumpStore(Store *store, StoreWriter *writer)
{
	LOG_INFO("Dumping store %s", store->name);

	StoreDumpContext *context = ALLOCATE_OBJECT(StoreDumpContext);
	context->store = store;
	context->writer = writer;
	context->level = 0;

	assert(store->root->type == STORE_ARRAY);
	g_hash_table_foreach(store->root->content.array, &dumpStoreNode, context);

	free(context);
}

/**
 * A GHFunc to dump a store node
 *
 * @param key_p		the key of the node
 * @param value_p	the value of the node
 * @param data		the dump's context
 */
static void dumpStoreNode(void *key_p, void *value_p, void *data)
{
	char *key = key_p;
	StoreNodeValue *value = value_p;
	StoreDumpContext *context = data;

	for(int i = 0; i < context->level; i++) {
		DUMP("\t");
	}

	GString *escaped = escapeStoreString(key);

	DUMP("\"%s\" = ", escaped->str);

	g_string_free(escaped, TRUE);

	dumpStoreNodeValue(value, context);

	DUMP("\n");
}

/**
 * Dumps a store node value
 *
 * @param value		the value of the node
 * @param context		the dump's context
 */
static void dumpStoreNodeValue(StoreNodeValue *value, StoreDumpContext *context)
{
	GString *escaped;

	switch (value->type) {
		case STORE_STRING:
			escaped = escapeStoreString(value->content.string);

			DUMP("\"%s\"", escaped->str);

			g_string_free(escaped, TRUE);
		break;
		case STORE_INTEGER:
			DUMP("%d", value->content.integer);
		break;
		case STORE_FLOAT_NUMBER:
			DUMP("%f", value->content.float_number);
		break;
		case STORE_LIST:
			DUMP("(");

			for (GList *iter = value->content.list->head; iter != NULL; iter = iter->next) {
				dumpStoreNodeValue(iter->data, context);

				if(iter->next != NULL) {
					DUMP(", ");
				}
			}

			DUMP(")");
		break;
		case STORE_ARRAY:
			DUMP("{\n");
			context->level++;
			g_hash_table_foreach(value->content.array, &dumpStoreNode, context);
			context->level--;

			for(int i = 0; i < context->level; i++) {
				DUMP("\t");
			}

			DUMP("}");
		break;
	}
}
