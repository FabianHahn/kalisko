/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Fabian "smf68" Hahn <smf68@smf68.ch>
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
#include <glib.h>
#include "dll.h"
#define API
#include "store.h"
#include "schema.h"

static SchemaType *createSchemaType(const char *name, SchemaTypeMode mode);
static SchemaStructElement *createSchemaStructElement(const char *key);
static void freeSchemaType(void *schemaType_p);
static void freeSchemaStructElement(void *schemaStructElement_p);

API Schema *parseSchema(Store *store)
{
	Schema *schema = ALLOCATE_OBJECT(Schema);
	schema->types = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaType);
	schema->rootElements = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaStructElement);

	return schema;
}

API void freeSchema(Schema *schema)
{
	free(schema);
}

/**
 * Creates a schema type with a certain mode
 *
 * @param name			the name of the schema type to create
 * @param mode			the mode of the schema type to create
 * @result				the created SchemaType object
 */
static SchemaType *createSchemaType(const char *name, SchemaTypeMode mode)
{
	SchemaType *schemaType = ALLOCATE_OBJECT(SchemaType);
	schemaType->name = strdup(name);
	schemaType->mode = mode;

	switch(schemaType->mode) {
		case SCHEMA_TYPE_MODE_INTEGER:
			schemaType->data.defaultInteger = 0;
		break;
		case SCHEMA_TYPE_MODE_FLOAT:
			schemaType->data.defaultFloat = 0.0;
		break;
		case SCHEMA_TYPE_MODE_STRING:
			schemaType->data.defaultString = NULL;
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			schemaType->data.structElements = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaStructElement);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
		case SCHEMA_TYPE_MODE_LIST:
			schemaType->data.subtype = NULL;
		break;
		case SCHEMA_TYPE_MODE_ENUM:
			schemaType->data.subtypes = g_queue_new();
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			assert(false); // TODO
		break;
		default:
			assert(false);
		break;
	}

	return schemaType;
}

/**
 * Creates a schema struct element
 *
 * @param key			the key of the schema struct element to create
 * @result				the created SchemaStructElement object
 */
static SchemaStructElement *createSchemaStructElement(const char *key)
{
	SchemaStructElement *element = ALLOCATE_OBJECT(SchemaStructElement);
	element->key = strdup(key);
	return element;
}

/**
 * Callback function to free a schema type
 *
 * @param schemaType_p			a pointer to the schema type
 */
static void freeSchemaType(void *schemaType_p)
{
	SchemaType *schemaType = schemaType_p;
	free(schemaType->name);

	switch(schemaType->mode) {
		case SCHEMA_TYPE_MODE_INTEGER:
		case SCHEMA_TYPE_MODE_FLOAT:
			// default number doesn't need to be freed
		break;
		case SCHEMA_TYPE_MODE_STRING:
			free(schemaType->data.defaultString);
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			g_hash_table_destroy(schemaType->data.structElements);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
		case SCHEMA_TYPE_MODE_LIST:
			// only one type pointer, nothing to free...
		break;
		case SCHEMA_TYPE_MODE_ENUM:
			g_queue_free(schemaType->data.subtypes);
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			assert(false); // TODO
		break;
		default:
			assert(false);
		break;
	}

	free(schemaType);
}

/**
 * Callback function to free a schema struct element
 *
 * @param schemaStructElement_p		a pointer to the schema struct element to free
 */
static void freeSchemaStructElement(void *schemaStructElement_p)
{
	SchemaStructElement *schemaStructElement = schemaStructElement_p;
	free(schemaStructElement->key);
	free(schemaStructElement);
}
