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

// Forward declaration
struct SchemaTypeStruct;

/**
 * Enum type specifiying what the matching mode of a schema type is
 */
typedef enum {
	SCHEMA_MODE_STRUCT,
	SCHEMA_MODE_ARRAY,
	SCHEMA_MODE_LIST,
	SCHEMA_MODE_ENUM,
	SCHEMA_MODE_VARIANT
} SchemaTypeMode;

/**
 * Union type storing data for schema type modes
 */
typedef union {
	/** Hash table connecting string keys to SchemaType subtype objects for struct mode */
	GHashTable *structTypes;
	/** Subtype for array and list mode */
	struct SchemaTypeStruct *subtype;
	/** Subtype list for enum mode */
	GQueue *subtypes;
	/** Custom data for variant mode */
	void *variantData; // TODO
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
 * Data type representing a schema element of a certain type
 */
typedef struct {
	char *key;
	SchemaType *type;
	bool required;
} SchemaElement;

/**
 * Data type representing a store schema that can be used to validate a store
 */
typedef struct {
	/** A hash table strings mapping to SchemaType objects */
	GHashTable *types;
	/** The root type of the schema */
	SchemaType *root;
} Schema;

#endif
