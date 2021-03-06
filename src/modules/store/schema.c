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
#include "write.h"

static SchemaType *parseSchemaType(Schema *schema, const char *name, Store *typeStore);
static SchemaType *parseSchemaTypeArray(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeSequence(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeTuple(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeVariant(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeEnum(Schema *schema, const char *name, GQueue *list);
static SchemaType *parseSchemaTypeStruct(Schema *schema, const char *name, GHashTable *array);
static SchemaStructElement *parseSchemaStructElement(Schema *schema, const char *key, GQueue *list);
static SchemaType *createSchemaType(const char *name, SchemaTypeMode mode);
static SchemaStructElement *createSchemaStructElement(const char *key);
static void freeSchemaType(void *schemaType_p);
static void freeSchemaStructElement(void *schemaStructElement_p);

API Schema *parseSchema(Store *store)
{
	if(store->type != STORE_ARRAY) {
		return NULL;
	}

	Schema *schema = ALLOCATE_OBJECT(Schema);
	schema->namedTypes = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaType);
	schema->anonymousTypes = g_ptr_array_new_with_free_func(&freeSchemaType);
	schema->layoutElements = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSchemaStructElement);

	// add default types
	g_hash_table_insert(schema->namedTypes, strdup("int"), createSchemaType("int", SCHEMA_TYPE_MODE_INTEGER));
	g_hash_table_insert(schema->namedTypes, strdup("float"), createSchemaType("float", SCHEMA_TYPE_MODE_FLOAT));
	g_hash_table_insert(schema->namedTypes, strdup("string"), createSchemaType("string", SCHEMA_TYPE_MODE_STRING));

	// parse schema types
	Store *types = g_hash_table_lookup(store->content.array, "types");

	if(types != NULL) {
		if(types->type != STORE_ARRAY) {
			logError("Failed to parse schema: 'types' section is not an array");
			freeSchema(schema); // invalid schema
			return NULL;
		}

		bool stuck = true;
		while(stuck) {
			stuck = false;
			int parsed = 0;

			GHashTableIter iter;
			char *name;
			Store *typeStore;
			g_hash_table_iter_init(&iter, types->content.array);
			while(g_hash_table_iter_next(&iter, (void *) &name, (void *) &typeStore)) {
				if(g_hash_table_lookup(schema->namedTypes, name) != NULL) {
					continue; // we already parsed this type
				}

				if(parseSchemaType(schema, name, typeStore)) {
					parsed++;
				} else {
					logInfo("Failed to parse schema type '%s'", name);
					stuck = true;
				}
			}

			if(stuck && parsed == 0) { // we are still stuck and made no progress
				logError("Failed to parse schema: Some types failed to parse");
				freeSchema(schema);
				return NULL;
			}
		}
	} else {
		logNotice("Schema does not contain a types section");
	}

	// parse root layout
	Store *layout;
	if((layout = g_hash_table_lookup(store->content.array, "layout")) == NULL) {
		logError("Failed to parse schema: No 'layout' section found");
		freeSchema(schema); // invalid schema
		return NULL;
	}

	if(layout->type != STORE_ARRAY) {
		logError("Failed to parse schema: 'layout' section is not an array");
		freeSchema(schema); // invalid schema
		return NULL;
	}

	GHashTableIter iter;
	char *key;
	Store *structElementStore;
	g_hash_table_iter_init(&iter, layout->content.array);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &structElementStore)) {
		if(structElementStore->type != STORE_LIST) {
			logError("Failed to parse schema: Layout element '%s' is not a valid struct element", key);
			freeSchema(schema);
			return NULL;
		}

		SchemaStructElement *structElement;
		if((structElement = parseSchemaStructElement(schema, key, structElementStore->content.list)) == NULL) {
			logError("Failed to parse schema: Failed to parse layout struct element '%s'", key);
			freeSchema(schema);
			return NULL;
		}

		g_hash_table_insert(schema->layoutElements, strdup(key), structElement);
	}

	return schema;
}

API void freeSchema(Schema *schema)
{
	g_hash_table_destroy(schema->layoutElements);
	g_ptr_array_unref(schema->anonymousTypes);
	g_hash_table_destroy(schema->namedTypes);
	free(schema);
}

/**
 * Parses a schema type from its store representation and adds it to the schema
 *
 * @param schema			the schema for which to parse the type and to add the type to
 * @param name				the name of the type to parse or NULL if parsing an anonymous type
 * @param typeStore			the store representation of the type
 * @result					the parsed SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaType(Schema *schema, const char *name, Store *typeStore)
{
	SchemaType *type = NULL;

	switch(typeStore->type) {
		case STORE_STRING:
		{
			// We are parsing an alias to a named type, that might or might not be parsed yet
			type = createSchemaType(name, SCHEMA_TYPE_MODE_ALIAS);
			type->data.alias = strdup(typeStore->content.string);
		}
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
				type = parseSchemaTypeArray(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "sequence") == 0) {
				type = parseSchemaTypeSequence(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "tuple") == 0) {
				type = parseSchemaTypeTuple(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "variant") == 0) {
				type = parseSchemaTypeVariant(schema, name, typeStore->content.list);
			} else if(g_strcmp0(identifier->content.string, "enum") == 0) {
				type = parseSchemaTypeEnum(schema, name, typeStore->content.list);
			} else {
				return NULL; // invalid type
			}
		}
		break;
		case STORE_ARRAY:
			type = parseSchemaTypeStruct(schema, name, typeStore->content.array);
		break;
		default:
			// Nothing to do, we will just return NULL
		break;
	}

	if(type != NULL) {
		if(name != NULL) { // add as named type
			g_hash_table_insert(schema->namedTypes, strdup(name), type);
			logNotice("Parsed named schema type '%s'", name);
		} else { // add as anonymous type
			assert(type->name == NULL);

			GString *typeString = writeStoreGString(typeStore);
			type->name = typeString->str; // steal the char representation
			g_string_free(typeString, false);

			g_ptr_array_add(schema->anonymousTypes, type);
			logNotice("Parsed anonymous schema type '%s'", type->name);
		}
	}

	return type;
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

	Store *subtypeStore = list->head->next->data;

	SchemaType *subtype = parseSchemaType(schema, NULL, subtypeStore);

	if(subtype != NULL) {
		SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_ARRAY);
		type->data.subtype = subtype;

		return type;
	} else {
		return NULL;
	}
}

/**
 * Parses a sequence schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param list				the store list representation of the sequence type
 * @result					the parsed sequence SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeSequence(Schema *schema, const char *name, GQueue *list)
{
	if(list->head->next == NULL) {
		return NULL; // invalid type
	}

	Store *subtypeStore = list->head->next->data;

	SchemaType *subtype = parseSchemaType(schema, NULL, subtypeStore);

	if(subtype != NULL) {
		SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_SEQUENCE);
		type->data.subtype = subtype;

		return type;
	} else {
		return NULL;
	}
}

/**
 * Parses a tuple schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param lsit				the store list representation of the tuple type
 * @result					the parsed tuple SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeTuple(Schema *schema, const char *name, GQueue *list)
{
	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_TUPLE);

	for(GList *iter = list->head->next; iter != NULL; iter = iter->next) {
		Store *subtypeStore = iter->data;

		SchemaType *subtype = parseSchemaType(schema, NULL, subtypeStore);

		if(subtype != NULL) {
			g_queue_push_tail(type->data.subtypes, subtype);
		} else {
			freeSchemaType(type);
			return NULL; // failed to parse subtype
		}
	}

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
		Store *subtypeStore = iter->data;

		SchemaType *subtype = parseSchemaType(schema, NULL, subtypeStore);

		if(subtype != NULL) {
			g_queue_push_tail(type->data.subtypes, subtype);
		} else {
			freeSchemaType(type);
			return NULL; // failed to parse subtype
		}
	}

	return type;
}

/**
 * Parses an enum schema type from its store representation
 *
 * @param schema			the schema for which to parse the type
 * @param name				the name of the type to parse
 * @param typeStore			the store list representation of the enum type
 * @result					the parsed enum SchemaType or NULL if the parse failed
 */
static SchemaType *parseSchemaTypeEnum(Schema *schema, const char *name, GQueue *list)
{
	SchemaType *type = createSchemaType(name, SCHEMA_TYPE_MODE_ENUM);

	for(GList *iter = list->head->next; iter != NULL; iter = iter->next) {
		Store *constantStore = iter->data;

		if(constantStore->type != STORE_STRING) {
			freeSchemaType(type); // expected string constant
			return NULL;
		}

		g_queue_push_tail(type->data.constants, strdup(constantStore->content.string));
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

	Store *subtypeStore = list->head->next->data;

	SchemaType *type = parseSchemaType(schema, NULL, subtypeStore);

	if(type != NULL) {
		structElement->type = type;
	} else {
		freeSchemaStructElement(structElement);
		return NULL; // could not parse type
	}

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
	schemaType->name = (name == NULL ? NULL : strdup(name));
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
		case SCHEMA_TYPE_MODE_SEQUENCE:
			schemaType->data.subtype = NULL;
		break;
		case SCHEMA_TYPE_MODE_TUPLE:
		case SCHEMA_TYPE_MODE_VARIANT:
			schemaType->data.subtypes = g_queue_new();
		break;
		case SCHEMA_TYPE_MODE_ALIAS:
			schemaType->data.alias = NULL;
		break;
		case SCHEMA_TYPE_MODE_ENUM:
			schemaType->data.constants = g_queue_new();
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
		case SCHEMA_TYPE_MODE_SEQUENCE:
			// only one type pointer, nothing to free...
		break;
		case SCHEMA_TYPE_MODE_TUPLE:
		case SCHEMA_TYPE_MODE_VARIANT:
			g_queue_free(schemaType->data.subtypes);
		break;
		case SCHEMA_TYPE_MODE_ALIAS:
			free(schemaType->data.alias);
		break;
		case SCHEMA_TYPE_MODE_ENUM:
			g_queue_free(schemaType->data.constants);
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
