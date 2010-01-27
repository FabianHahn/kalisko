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
#define DUMP(FORMAT, ...) context->writer(context, FORMAT, ##__VA_ARGS__)

static void storeFileWrite(void *context_p, char *format, ...);
static void storeGStringWrite(void *context_p, char *format, ...);
static void dumpStore(Store *value, StoreDumpContext *context);
static void dumpStoreNode(void *key_p, void *value_p, void *data);

/**
 * Writes a store from memory to a file
 *
 * @param filename		the filename to write to
 * @param store			the store to write
 */
API void writeStoreFile(char *filename, Store *store)
{
	StoreDumpContext context;
	context.resource = fopen(filename, "w");
	context.writer = &storeFileWrite;
	context.level = -1;

	LOG_DEBUG("Writing store to %s", filename);
	dumpStore(store, &context);

	fclose(context.resource);
}

/**
 * Writes a store from memory to a GString
 *
 * @param store		the store to write
 * @result			the written store string, must be freed with g_string_free
 */
API GString *writeStoreGString(Store *store)
{
	StoreDumpContext context;
	context.resource = g_string_new("");
	context.writer = &storeGStringWrite;
	context.level = -1;

	LOG_DEBUG("Writing store to string");
	dumpStore(store, &context);

	return context.resource;
}


/**
 * A StoreWriter function for files
 *
 * @param store		the store to to write to
 * @param format		printf-like string to write
 */
static void storeFileWrite(void *context_p, char *format, ...)
{
	StoreDumpContext *context = context_p;

	va_list va;
	va_start(va, format);

	vfprintf(context->resource, format, va);
}

/**
 * A StoreWriter function for GStrings
 *
 * @param store		the store to to write to
 * @param format		printf-like string to write
 */
static void storeGStringWrite(void *context_p, char *format, ...)
{
	StoreDumpContext *context = context_p;

	va_list va;
	va_start(va, format);

	g_string_append_vprintf(context->resource, format, va);
}


/**
 * Dumps a store
 *
 * @param value			the current store value to consider
 * @param context		the dump's context
 */
static void dumpStore(Store *value, StoreDumpContext *context)
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
				dumpStore(iter->data, context);

				if(iter->next != NULL) {
					DUMP(", ");
				}
			}

			DUMP(")");
		break;
		case STORE_ARRAY:
			if(context->level >= 0) {
				DUMP("{\n");
			}

			context->level++;
			g_hash_table_foreach(value->content.array, &dumpStoreNode, context);
			context->level--;

			for(int i = 0; i < context->level; i++) {
				DUMP("\t");
			}

			if(context->level >= 0) {
				DUMP("}");
			}
		break;
	}
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
	Store *value = value_p;
	StoreDumpContext *context = data;

	for(int i = 0; i < context->level; i++) {
		DUMP("\t");
	}

	GString *escaped = escapeStoreString(key);

	DUMP("\"%s\" = ", escaped->str);

	g_string_free(escaped, TRUE);

	dumpStore(value, context);

	DUMP("\n");
}
