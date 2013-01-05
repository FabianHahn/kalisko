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

#define API
#include "store.h"
#include "write.h"

/**
 * A store writer to write down a store from memory
 * Note: The first param has only type void * and not StoreDumpContext to get around C's single pass compilation restrictions*
 */
typedef void (StoreWriter)(void *context_p, char *format, ...);

/**
 * A helper struct that's used to dump stores
 */
typedef struct {
	/** the resource to dump to */
	void *resource;
	/** the store writer to use for dumping */
	StoreWriter *writer;
	/** the current indentation level */
	int level;
	/** true if the next leading newline should be skipped */
	bool skip_newline;
} StoreDumpContext;

/**
 * Convenience macro to write to store dumps
 *
 * @param FORMAT		printf-like format string
 */
#define DUMP(FORMAT, ...) context->writer(context, FORMAT, ##__VA_ARGS__)

static void storeFileWrite(void *context_p, char *format, ...) G_GNUC_PRINTF(2,3);
static void storeGStringWrite(void *context_p, char *format, ...) G_GNUC_PRINTF(2,3);
static void dumpStore(Store *value, StoreDumpContext *context);
static void dumpStoreNode(void *key_p, void *value_p, void *data);

API bool writeStoreFile(const char *filename, Store *store)
{
	StoreDumpContext context;
	if((context.resource = fopen(filename, "w")) == NULL) {
		LOG_SYSTEM_ERROR("Failed to open file '%s' to write store", filename);
		return false;
	}

	context.writer = &storeFileWrite;
	context.level = -1;

	dumpStore(store, &context);

	fclose(context.resource);

	return true;
}

API GString *writeStoreGString(Store *store)
{
	StoreDumpContext context;
	context.resource = g_string_new("");
	context.writer = &storeGStringWrite;
	context.level = -1;

	dumpStore(store, &context);

	return context.resource;
}


/**
 * A StoreWriter function for files
 *
 * @param context_p		a pointer to the store dump context to write to
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
 * @param context_p		a pointer to the store dump context to write to
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

			context->skip_newline = true;
			context->level++;
			g_hash_table_foreach(value->content.array, &dumpStoreNode, context);
			context->level--;
			context->skip_newline = false;

			if(context->level >= 0) {
				DUMP("\n");

				for(int i = 0; i < context->level; i++) {
					DUMP("\t");
				}

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

	if(context->skip_newline) {
		context->skip_newline = false;
	} else {
		DUMP("\n");
	}

	for(int i = 0; i < context->level; i++) {
		DUMP("\t");
	}

	GString *escaped = escapeStoreString(key);

	DUMP("\"%s\" = ", escaped->str);

	g_string_free(escaped, TRUE);

	dumpStore(value, context);
}
