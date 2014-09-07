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
#include "modules/string_util/string_util.h"
#define API
#include "store.h"
#include "schema.h"
#include "validate.h"

static bool validateSchemaType(Schema *schema, SchemaType *type, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeInteger(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeFloat(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeString(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeStruct(Schema *schema, const char *typeName, GHashTable *structElementTable, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeArray(Schema *schema, const char *typeName, SchemaType *subtype, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeSequence(Schema *schema, const char *typeName, SchemaType *subtype, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeTuple(Schema *schema, const char *typeName, GQueue *subtype, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeVariant(Schema *schema, const char *typeName, GQueue *subtypes, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeAlias(Schema *schema, const char *typeName, char *alias, Store *store, const char *storePath, GString *errorString);
static bool validateSchemaTypeEnum(Schema *schema, const char *typeName, GQueue *subtypes, Store *store, const char *storePath, GString *errorString);

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
	GString *storePath = g_string_new("");
	GString *errorString = g_string_new("");

	bool validated = validateSchemaTypeStruct(schema, "[schema root layout]", schema->layoutElements, store, storePath->str, errorString);

	if(!validated) {
		logError("Failed to validate schema:%s", errorString->str);
	}

	g_string_free(storePath, true);
	g_string_free(errorString, true);

	return validated;
}

/**
 * Validate a schema type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param type					the schema type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated*
 * @result true					true if validation passes
 */
static bool validateSchemaType(Schema *schema, SchemaType *type, Store *store, const char *storePath, GString *errorString)
{
	bool validation = true;

	switch(type->mode) {
		case SCHEMA_TYPE_MODE_INTEGER:
			validation &= validateSchemaTypeInteger(schema, type->name, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_FLOAT:
			validation &= validateSchemaTypeFloat(schema, type->name, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_STRING:
			validation &= validateSchemaTypeString(schema, type->name, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_STRUCT:
			validation &= validateSchemaTypeStruct(schema, type->name, type->data.structElements, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_ARRAY:
			validation &= validateSchemaTypeArray(schema, type->name, type->data.subtype, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_SEQUENCE:
			validation &= validateSchemaTypeSequence(schema, type->name, type->data.subtype, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_TUPLE:
			validation &= validateSchemaTypeTuple(schema, type->name, type->data.subtypes, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_VARIANT:
			validation &= validateSchemaTypeVariant(schema, type->name, type->data.subtypes, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_ALIAS:
			validation &= validateSchemaTypeAlias(schema, type->name, type->data.alias, store, storePath, errorString);
		break;
		case SCHEMA_TYPE_MODE_ENUM:
			validation &= validateSchemaTypeEnum(schema, type->name, type->data.constants, store, storePath, errorString);
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
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeInteger(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_INTEGER) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not an integer, but should be of integer type '%s'", storePath, typeName);
		return false;
	}

	return true;
}

/**
 * Validate a float type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeFloat(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_FLOAT_NUMBER) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not a float, but should be of float type '%s'", storePath, typeName);
		return false;
	}

	return true;
}

/**
 * Validate a string type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeString(Schema *schema, const char *typeName, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_STRING) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not a string, but should be of string type '%s'", storePath, typeName);
		return false;
	}

	return true;
}

/**
 * Validate a struct type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param structElementTable	a hash table of SchemaStructElement objects comprising the schema struct type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeStruct(Schema *schema, const char *typeName, GHashTable *structElementTable, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_ARRAY) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not an array, but should be of struct type '%s'", storePath, typeName);
		return false;
	}

	GHashTableIter iter;
	char *key;
	SchemaStructElement *schemaStructElement;
	g_hash_table_iter_init(&iter, structElementTable);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &schemaStructElement)) {
		Store *storeStructElement = g_hash_table_lookup(store->content.array, key);

		if(storeStructElement == NULL && schemaStructElement->required) {
			g_string_append_printf(errorString, "\nStore element at '%s/%s' of struct type '%s' is required, but was not found", storePath, key, typeName);
			return false;
		}

		if(storeStructElement != NULL) {
			GString *storePathAppended = g_string_new(storePath);
			g_string_append_printf(storePathAppended, "/%s", key);

			bool validated = validateSchemaType(schema, schemaStructElement->type, storeStructElement, storePathAppended->str, errorString);
			g_string_free(storePathAppended, true);

			if(!validated) {
				return false;
			}
		}
	}

	return true;
}

/**
 * Validate a array type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param subtype				the subtype of the schema array type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeArray(Schema *schema, const char *typeName, SchemaType *subtype, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_ARRAY) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not an array, but should be of array type '%s'", storePath, typeName);
		return false;
	}

	GHashTableIter iter;
	char *key;
	Store *arrayElement;
	g_hash_table_iter_init(&iter, store->content.array);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &arrayElement)) {
		GString *storePathAppended = g_string_new(storePath);
		g_string_append_printf(storePathAppended, "/%s", key);

		bool validated = validateSchemaType(schema, subtype, arrayElement, storePathAppended->str, errorString);
		g_string_free(storePathAppended, true);

		if(!validated) {
			return false;
		}
	}

	return true;
}

/**
 * Validate a sequence type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param subtype				the subtype of the sequence schema type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeSequence(Schema *schema, const char *typeName, SchemaType *subtype, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_LIST) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not a list, but should be of sequence type '%s'", storePath, typeName);
		return false;
	}

	int i = 0;

	for(GList *iter = store->content.list->head; iter != NULL; iter = iter->next, i++) {
		Store *listElement = iter->data;

		GString *storePathAppended = g_string_new(storePath);
		g_string_append_printf(storePathAppended, "/%d", i);

		bool validated = validateSchemaType(schema, subtype, listElement, storePathAppended->str, errorString);
		g_string_free(storePathAppended, true);

		if(!validated) {
			return false;
		}
	}

	return true;
}

/**
 * Validate a tuple type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param subtypes				the subtype list of the tuple schema type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeTuple(Schema *schema, const char *typeName, GQueue *subtypes, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_LIST) {
		g_string_append_printf(errorString, "\nStore element at '%s' is not a list, but should be of tuple type '%s'", storePath, typeName);
		return false;
	}

	GList *storeIter = store->content.list->head;
	int i = 0;

	for(GList *subtypeIter = subtypes->head; subtypeIter != NULL; subtypeIter = subtypeIter->next, storeIter = storeIter->next, i++) {
		SchemaType *subtype = subtypeIter->data;

		if(storeIter == NULL) {
			g_string_append_printf(errorString, "\nStore element at '%s/%d' not set, but should be of tuple type '%s' subtype '%s'", storePath, i, typeName, subtype->name);
			return false;
		}

		Store *tupleElement = storeIter->data;

		GString *storePathAppended = g_string_new(storePath);
		g_string_append_printf(storePathAppended, "/%d", i);

		bool validated = validateSchemaType(schema, subtype, tupleElement, storePathAppended->str, errorString);
		g_string_free(storePathAppended, true);

		if(!validated) {
			g_string_append_printf(errorString, "\nStore element at '%s/%d' should be of tuple type '%s' subtype '%s'", storePath, i, typeName, subtype->name);
			return false;
		}
	}

	return true;
}

/**
 * Validate a variant type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param subtypes				the subtypes list of the schema variant type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeVariant(Schema *schema, const char *typeName, GQueue *subtypes, Store *store, const char *storePath, GString *errorString)
{
	GString *variantErrorString = g_string_new("");
	g_string_append_printf(variantErrorString, "\nStore element at '%s' does not match any of the variant subtypes of type '%s':", storePath, typeName);

	for(GList *iter = subtypes->head; iter != NULL; iter = iter->next) {
		SchemaType *subtype = iter->data;

		GString *variantAttemptErrorString = g_string_new("");

		bool validated = validateSchemaType(schema, subtype, store, storePath, variantAttemptErrorString);

		char *indented = indentString(variantAttemptErrorString->str, "\t", 2);

		g_string_append_printf(variantErrorString, "\n\tAttempting to parse as variant subtype '%s':%s", subtype->name, indented);

		g_string_free(variantAttemptErrorString, true);
		free(indented);

		if(validated) {
			g_string_free(variantErrorString, true);
			return true;
		}
	}

	g_string_append(errorString, variantErrorString->str);
	g_string_free(variantErrorString, true);
	return false;
}

/**
 * Validate an alias type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param alias					the alias string of the schema alias type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeAlias(Schema *schema, const char *typeName, char *alias, Store *store, const char *storePath, GString *errorString)
{
	SchemaType *aliasedType = g_hash_table_lookup(schema->namedTypes, alias);

	if(aliasedType == NULL) {
		logWarning("Validating alias type '%s' referring to non-existing type '%s'", typeName, alias);
		return true;
	} else {
		return validateSchemaType(schema, aliasedType, store, storePath, errorString);
	}
}

/**
 * Validate an enum type within a schema against a given store
 *
 * @param schema				the schema that we are validating
 * @param typeName				the name of the type we are validating
 * @param subtypes				the constants list of the schema enum type which we want to validate
 * @param store					the store we are validating
 * @param storePath				path location whithin the store we are validating
 * @param errorString			error string into which parse errors are accumulated
 * @result true					true if validation passes
 */
static bool validateSchemaTypeEnum(Schema *schema, const char *typeName, GQueue *constants, Store *store, const char *storePath, GString *errorString)
{
	if(store->type != STORE_STRING) {
		g_string_append_printf(errorString, "\nStore element at '%s' should be an enum constant of type '%s', but is not a string!", storePath, typeName);
		return false;
	}

	for(GList *iter = constants->head; iter != NULL; iter = iter->next) {
		char *constant = iter->data;

		if(g_strcmp0(store->content.string, constant) == 0) {
			return true;
		}
	}

	g_string_append_printf(errorString, "\nStore element at '%s' should be an enum constant of type '%s', but is '%s'", storePath, typeName, store->content.string);
	return false;
}
