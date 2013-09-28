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

static SchemaType *parseSchemaType(Schema *schema, const char *name, Store *typeStore);
static SchemaType *parseSchemaTypeArray(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeList(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeVariant(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeStruct(Schema *schema, const char *name, GHashTable *array);
static SchemaStructElement *parseSchemaStructElement(Schema *schema, const char *key, GQueue *list);
static SchemaType *createSchemaType(const char *name, SchemaTypeMode mode);
static SchemaStructElement *createSchemaStructElement(const char *key);
static void freeSchemaType(void *schemaType_p);
static void freeSchemaStructElement(void *schemaStructElement_p);

API Schema *parseSchema(Store *store)
{
	Schema *schema = ALLOCATE_OBJECT(Schema);
	schema->types = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaType);
	schema->rootElements = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaStructElement);

	// add default types
	g_hash_table_insert(schema->types, strdup("int"), createSchemaType("int", SCHEMA_TYPE_MODE_INTEGER));
	g_hash_table_insert(schema->types, strdup("float"), createSchemaType("float", SCHEMA_TYPE_MODE_FLOAT));
	g_hash_table_insert(schema->types, strdup("string"), createSchemaType("string", SCHEMA_TYPE_MODE_STRING));

	return schema;
}

API void freeSchema(Schema *schema)
{
	free(schema);
}

/**
 * Parses a schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param typeStore			the store representation of the type
 * @result					the parsed SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaType(Schema *schema, const char *name, Store *typeStore)
{
	switch(typeStore->type) {
		case STORE_STRING:
		case STORE_INTEGER:
		case STORE_FLOAT_NUMBER:
			return NULL; // invalid type
		break;
		case STORE_LIST:
		{
			if(g_queue_get_length(typeStore->content.list) == 0) {
				return NULL; // invalid type
			}

			Store *identifier = g_queue_peek_head(typeStore->content.list);
			if(identifier->type != STORE_STRING) {
				return NULL; // invalid type
			}

			if(g_strcmp0(identifier->content.string, "array") == 0) {
				return parseSchemaTypeArray(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "list") == 0) {
				return parseSchemaTypeList(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "variant") == 0) {
				return parseSchemaTypeVariant(schema, name, typeStore->content.list);
			} else {
				return NULL; // invalid type
			}
		}
		break;
		case STORE_ARRAY:
			return parseSchemaTypeStruct(schema, name, typeStore->content.array);
		break;
		default:
			assert(false);
		break;
	}

	return NULL;
}

/**
 * Parses an array schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param typeStore			the store list representation of the array type
 * @result					the parsed array SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeArray(Schema *schema, const char *name, GQueue *list)
{
	if(list->head->next == NULL) {
		return NULL; // invalid type
	}

	Store *typeName = list->head->next->data;

	if(typeName->type != STORE_STRING) {
		return NULL; // invalid type
	}

	SchemaType *subtype;
	if((subtype = g_hash_table_lookup(schema->types, typeName->content.string)) == NULL) {
		return NULL; // subtype not found
	}

	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_ARRAY);
	type->data.subtype = subtype;

	return type;
}

/**
 * Parses an list schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param typeStore			the store list representation of the list type
 * @result					the parsed list SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeList(Schema *schema, const char *name, GQueue *list)
{
	if(list->head->next == NULL) {
		return NULL; // invalid type
	}

	Store *typeName = list->head->next->data;

	if(typeName->type != STORE_STRING) {
		return NULL; // invalid type
	}

	SchemaType *subtype;
	if((subtype = g_hash_table_lookup(schema->types, typeName->content.string)) == NULL) {
		return NULL; // subtype not found
	}

	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_LIST);
	type->data.subtype = subtype;

	return type;
}

/**
 * Parses an variant schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param typeStore			the store list representation of the variant type
 * @result					the parsed variant SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeVariant(Schema *schema, const char *name, GQueue *list)
{
	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_VARIANT);

	for(GList *iter = list->head->next; iter != NULL; iter = iter->next) {
		Store *typeName = iter->data;

		if(typeName->type != STORE_STRING) {
			freeSchemaType(type);
			return NULL; // invalid type
		}

		SchemaType *subtype;
		if((subtype = g_hash_table_lookup(schema->types, typeName->content.string)) == NULL) {
			freeSchemaType(type);
			return NULL; // subtype not found
		}

		g_queue_push_tail(type->data.subtypes, subtype);
	}

	return type;
}

/**
 * Parses an struct schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param array				the store array representation of the struct type
 * @result					the parsed struct SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeStruct(Schema *schema, const char *name, GHashTable *array)
{
	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_STRUCT);

	GHashTableIter iter;
	char *key;
	Store *structElementStore;
	g_hash_table_iter_init(&iter, array);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &structElementStore)) {
		if(structElementStore->type != STORE_LIST) {
			freeSchemaType(type);
			return NULL; // invalid type
		}

		SchemaStructElement *structElement;
		if((structElement = parseSchemaStructElement(schema, key, structElementStore->content.list)) == NULL) {
			freeSchemaType(type);
			return NULL; // invalid type
		}

		g_hash_table_insert(type->data.structElements, strdup(key), structElement);
	}

	return type;
}

/**
 * Parses a struct element from its store representation
 *
 * @param schema			the schema for which to parse the struct element
 * @param key				the key of the struct element to parse
 * @param list				the store list representation of the struct element
 * @result					the parsed SchemaStructElement or NULL if the parse failed
 */
static SchemaStructElement *parseSchemaStructElement(Schema *schema, const char *key, GQueue *list)
{
	SchemaStructElement *structElement = createSchemaStructElement(key);

	if(list->head == NULL) {
		freeSchemaStructElement(structElement);
		return NULL; // invalid type
	}

	Store *requiredFlag = list->head->data;

	if(requiredFlag->type != STORE_STRING) {
		freeSchemaStructElement(structElement);
		return NULL; // invalid type
	}

	structElement->required = (g_strcmp0(requiredFlag->content.string, "required") == 0);

	if(list->head->next == NULL) {
		freeSchemaStructElement(structElement);
		return NULL; // invalid type
	}

	Store *typeName = list->head->next->data;

	SchemaType *type;
	if((type = g_hash_table_lookup(schema->types, typeName->content.string)) == NULL) {
		freeSchemaStructElement(structElement);
		return NULL; // type not found
	}

	structElement->type = type;

	return structElement;
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
		case SCHEMA_TYPE_MODE_FLOAT:
		case SCHEMA_TYPE_MODE_STRING:
			// nothing to do
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			schemaType->data.structElements = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaStructElement);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
		case SCHEMA_TYPE_MODE_LIST:
			schemaType->data.subtype = NULL;
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			schemaType->data.subtypes = g_queue_new();
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
		case SCHEMA_TYPE_MODE_STRING:
			// nothing to do
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			g_hash_table_destroy(schemaType->data.structElements);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
		case SCHEMA_TYPE_MODE_LIST:
			// only one type pointer, nothing to free...
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			g_queue_free(schemaType->data.subtypes);
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
