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

#include "dll.h"
#define API
#include "store.h"
#include "schema.h"
#include "validate.h"

static bool validateSchemaInteger(const char *typeName, Store *store);
static bool validateSchemaFloat(const char *typeName, Store *store);
static bool validateSchemaString(const char *typeName, Store *store);
static bool validateSchemaStruct(const char *typeName, GHashTable *structElementTable, Store *store);
static bool validateSchemaArray(const char *typeName, SchemaType *subtype, Store *store);
static bool validateSchemaList(const char *typeName, SchemaType *subtype, Store *store);
static bool validateSchemaVariant(const char *typeName, GQueue *subtypes, Store *store);

API bool validateStoreByStoreSchema(Store *store, Store *schemaStore)
{
	Schema *schema;
	if((schema = parseSchema(schemaStore)) == NULL) {
		return false;
	}

	bool result = validateStore(store, schema);

	freeSchema(schema);

	return result;
}

API bool validateStore(Store *store, Schema *schema)
{
	return validateSchemaStruct("[schema root layout]", schema->layoutElements, store);
}

/**
 * Validate a schema type within a schema against a given store
 *
 * @param type					the schema type which we want to validate
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaType(SchemaType *type, Store *store)
{
	bool validation = true;

	switch(type->mode) {
		case SCHEMA_TYPE_MODE_INTEGER:
			validation &= validateSchemaInteger(type->name, store);
		break;
		case SCHEMA_TYPE_MODE_FLOAT:
			validation &= validateSchemaFloat(type->name, store);
		break;
		case SCHEMA_TYPE_MODE_STRING:
			validation &= validateSchemaString(type->name, store);
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			validation &= validateSchemaStruct(type->name, type->data.structElements, store);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
			validation &= validateSchemaArray(type->name, type->data.subtype, store);
		break;
		case SCHEMA_TYPE_MODE_LIST:
			validation &= validateSchemaList(type->name, type->data.subtype, store);
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			validation &= validateSchemaVariant(type->name, type->data.subtypes, store);
		break;
		default:
			assert(false);
		break;
	}

	return validation;
}

/**
 * Validate an integer type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaInteger(const char *typeName, Store *store)
{
	if(store->type != STORE_INTEGER) {
		logError("Failed to validate store: Store element is not an integer, but should be integer type '%s'", typeName);
		return false;
	}

	return true;
}

/**
 * Validate a float type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaFloat(const char *typeName, Store *store)
{
	if(store->type != STORE_FLOAT_NUMBER) {
		logError("Failed to validate store: Store element is not a float, but should be float type '%s'", typeName);
		return false;
	}

	return true;
}

/**
 * Validate a string type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaString(const char *typeName, Store *store)
{
	if(store->type != STORE_STRING) {
		logError("Failed to validate store: Store element is not a string, but should be string type '%s'", typeName);
		return false;
	}

	return true;
}

/**
 * Validate a struct type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param structElementTable	a hash table of SchemaStructElement objects comprising the schema struct type which we want to validate
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaStruct(const char *typeName, GHashTable *structElementTable, Store *store)
{
	if(store->type != STORE_ARRAY) {
		logError("Failed to validate store: Store element is not an array, but should be struct type '%s'", typeName);
		return false;
	}

	GHashTableIter iter;
	char *key;
	SchemaStructElement *schemaStructElement;
	g_hash_table_iter_init(&iter, structElementTable);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &schemaStructElement)) {
		Store *storeStructElement = g_hash_table_lookup(store->content.array, key);

		if(storeStructElement == NULL && schemaStructElement->required) {
			logError("Failed to validate store: Key '%s' of struct type '%s' required but not found", key, typeName);
			return false;
		}

		if(storeStructElement != NULL) {
			if(!validateSchemaType(schemaStructElement->type, storeStructElement)) {
				return false;
			}
		}
	}

	return true;
}

/**
 * Validate a array type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param subtype				the subtype of the schema array type which we want to validate
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaArray(const char *typeName, SchemaType *subtype, Store *store)
{

	return true;
}

/**
 * Validate a list type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param subtype				the subtype of the schema list type which we want to validate
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaList(const char *typeName, SchemaType *subtype, Store *store)
{
	return true;
}

/**
 * Validate a variant type within a schema against a given store
 *
 * @param typeName				the name of the type we are validating
 * @param subtypes				the subtypes list of the schema variant type which we want to validate
 * @param store					the store we are validating
 * @result true					true if validation passes
 */
static bool validateSchemaVariant(const char *typeName, GQueue *subtypes, Store *store)
{
	return true;
}
