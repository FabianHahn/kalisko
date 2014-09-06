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

#ifndef STORE_SCHEMA_H
#define STORE_SCHEMA_H

#include "store.h"

// Forward declaration
struct SchemaTypeStruct;

/**
 * Enum type specifiying what the matching mode of a schema type is
 */
typedef enum {
	SCHEMA_TYPE_MODE_INTEGER,
	SCHEMA_TYPE_MODE_FLOAT,
	SCHEMA_TYPE_MODE_STRING,
	SCHEMA_TYPE_MODE_STRUCT,
	SCHEMA_TYPE_MODE_ARRAY,
	SCHEMA_TYPE_MODE_LIST,
	SCHEMA_TYPE_MODE_VARIANT,
	SCHEMA_TYPE_MODE_ALIAS,
	SCHEMA_TYPE_MODE_ENUM
} SchemaTypeMode;

/**
 * Union type storing data for schema type modes
 */
typedef union {
	/** Hash table connecting string keys to SchemaStructElement objects for struct mode */
	GHashTable *structElements;
	/** Subtype for array and list mode */
	struct SchemaTypeStruct *subtype;
	/** Subtype list for variant mode */
	GQueue *subtypes;
	/** Aliased type name for alias mode */
	char *alias;
	/** String constant list for enum mode */
	GQueue *constants;
} SchemaTypeModeData;

/**
 * Data type representing a schema value type
 */
struct SchemaTypeStruct {
	char *name;
	SchemaTypeMode mode;
	SchemaTypeModeData data;
};

typedef struct SchemaTypeStruct SchemaType;

/**
 * Data type representing a schema element of a struct mode schema type
 */
typedef struct {
	char *key;
	SchemaType *type;
	bool required;
} SchemaStructElement;

/**
 * Data type representing a store schema that can be used to validate a store
 */
typedef struct {
	/** A hash table strings mapping to SchemaType objects representing names types of the schema */
	GHashTable *namedTypes;
	/** An array of SchemaType objects representing anonymous types of the schema */
	GPtrArray *anonymousTypes;
	/** A table of SchemaStructElement objects representing the layout of the schema */
	GHashTable *layoutElements;
} Schema;

/**
 * Parses a schema from its store representation
 *
 * @param store			the store from which to parse the schema
 * @result				the parsed schema or NULL on error
 */
API Schema *parseSchema(Store *store);

/**
 * Frees a schema
 *
 * @param schema		the schema to free
 */
API void freeSchema(Schema *schema);

#endif
