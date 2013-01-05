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

#ifndef STORE_STORE_H
#define STORE_STORE_H

#include <glib.h>

/**
 * Enumeration of the store value types
 */
typedef enum {
	/** A string value */
	STORE_STRING,
	/** An integer value */
	STORE_INTEGER,
	/** A floating point number value */
	STORE_FLOAT_NUMBER,
	/** A list value */
	STORE_LIST,
	/** An associative array value */
	STORE_ARRAY
} StoreValueType;

/**
 * Union to store a node value's content
 */
typedef union {
	/** A string value */
	char *string;
	/** An integer value */
	int integer;
	/** A floating point number value */
	double float_number;
	/** A list value */
	GQueue *list;
	/** An associative array value */
	GHashTable *array;
} StoreValueContent;

/**
 * Struct to represent a store, respectively a node value in the tree
 */
typedef struct {
	/** The node value's type */
	StoreValueType type;
	/** The node value's content */
	StoreValueContent content;
} Store;

/**
 * Struct to represent a store node
 * Note: This struct is only used internally to do the parsing, it is NOT used in the final parsed store's representation
 */
typedef struct {
	/** The node's key */
	char *key;
	/** The node's value, another store */
	Store *value;
} StoreNode;


/**
 * Creates an empty store
 *
 * @result			the created store
 */
API Store *createStore();

/**
 * A GDestroyNotify function to free a store node value
 *
 * @param store		the store node value to free
 */
API void freeStore(void *store);

/**
 * Returns a node value's actual content
 *
 * @param value		the store node value
 * @result 			the node value's content
 */
API void *getStoreValueContent(Store *value);

/**
 * Escapes a store string for output in a dump
 *
 * @param string		the string to escape
 * @result				the escaped string, must be freed with g_string_free
 */
API GString *escapeStoreString(char *string);

/**
 * Creates a string value to be used in a store
 *
 * @param string		the content string
 * @result				the created node value, must be freed with freeStore or by the store system
 */
API Store *createStoreStringValue(const char *string);

/**
 * Creates an integer value to be used in a store
 *
 * @param integer		the content integer
 * @result				the created node value, must be freed with freeStore or by the store system
 */
API Store *createStoreIntegerValue(int integer);

/**
 * Creates a float number value to be used in a store
 *
 * @param float_number	the content float number
 * @result				the created node value, must be freed with freeStore or by the store system
 */
API Store *createStoreFloatNumberValue(double float_number);

/**
 * Creates a list value to be used in a store
 *
 * @param list			the content list or NULL for an empty list
 * @result				the created node value, must be freed with freeStore or by the store system
 */
API Store *createStoreListValue(GQueue *list);

/**
 * Creates an array value to be used in a store
 *
 * @param array			the content array or NULL if an empty one should be created
 * @result				the created node value, must be freed with freeStore or by the store system
 */
API Store *createStoreArrayValue(GHashTable *array);

/**
 * Creates an empty store nodes table to be used as a section or an array in a store
 *
 * @result			the created nodes table, must be freed with g_hash_table_destroy or the store system
 */
API GHashTable *createStoreNodes();

#endif
