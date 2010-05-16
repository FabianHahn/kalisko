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

API Store *createStore();
API void freeStore(void *store);
API void *getStoreValueContent(Store *value);
API GString *escapeStoreString(char *string);
API Store *createStoreStringValue(char *string);
API Store *createStoreIntegerValue(int integer);
API Store *createStoreFloatNumberValue(double float_number);
API Store *createStoreListValue(GQueue *list);
API Store *createStoreArrayValue(GHashTable *array);
API GHashTable *createStoreNodes();

#endif
