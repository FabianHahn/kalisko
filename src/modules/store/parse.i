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

#ifndef STORE_PARSE_H
#define STORE_PARSE_H

#include <glib.h>
#include <stdio.h>
#include "store.h"

/**
 * Parser union for bison
 */
typedef union
{
	char *string;
	int integer;
	double float_number;
	Store *value;
	StoreNode *node;
} YYSTYPE;

/**
 * A store reader to retrieve characters from a source
 * Note: The first param has only type void * and not StoreParser to get around C's single pass compilation restrictions
 */
typedef char ( StoreReader)(void *parser);

/**
 * A store unreader to push back characters into a source
 * Note: The first param has only type void * and not StoreParser to get around C's single pass compilation restrictions
 */
typedef void ( StoreUnreader)(void *parser, char c);

/**
 * Struct to represent a store
 */
typedef struct
{
	union
	{
		/** The store parser's resource */
		void *resource;
		/** The store parser's const resource */
		const void *const_resource;
	};
	/** The store's reader */
	StoreReader *read;
	/** The store's unreader */
	StoreUnreader *unread;
	/** The store to parse to */
	Store *store;
} StoreParser;

/**
 * A store string's maximum length
 */
#define STORE_MAX_STRING_LENGTH 1024

API Store *parseStoreFile(const char *filename);
API Store *parseStoreString(const char *string);
API char storeFileRead(void *parser_p);
API void storeFileUnread(void *parser_p, char c);
API char storeStringRead(void *parser_p);
API void storeStringUnread(void *parser_p, char c);

#endif