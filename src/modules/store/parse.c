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

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"
#include "log.h"

#include "api.h"
#include "parse.h"
#include "parser.h"
#include "lexer.h"

int yyparse(Store *store); // this can't go into a header because it doesn't have an API export

/**
 * Parses a store file
 *
 * @param filename		the file name of the store file to parse
 * @result				the parsed store
 */
API Store *parseStoreFile(char *filename)
{
	Store *store = ALLOCATE_OBJECT(Store);

	store->name = strdup(filename);
	store->resource = fopen(store->name, "r");
	store->read = &storeFileRead;
	store->unread = &storeFileUnread;

	if(store->resource == NULL) {
		LOG_SYSTEM_ERROR("Could not open store file %s", store->name);
		free(store->name);
		free(store);
		return NULL;
	}

	LOG_INFO("Parsing store file %s", store->name);

	if(yyparse(store) != 0) {
		LOG_ERROR("Parsing store file %s failed", store->name);
		free(store->name);
		fclose(store->resource);
		free(store);
		return NULL;
	}

	fclose(store->resource);

	return store;
}

/**
 * Parses a store string
 *
 * @param string		the store string to parse
 * @result				the parsed store
 */
API Store *parseStoreString(char *string)
{
	Store *store = ALLOCATE_OBJECT(Store);

	store->name = strdup(string);
	store->resource = store->name;
	store->read = &storeStringRead;
	store->unread = &storeStringUnread;

	LOG_INFO("Parsing store string: %s", store->name);

	if(yyparse(store) != 0) {
		LOG_ERROR("Parsing store string failed");
		free(store->name);
		free(store);
		return NULL;
	}

	return store;
}

/**
 * A StoreReader function for files
 *
 * @param store		the store to to read from
 * @result				the read character
 */
API char storeFileRead(void *store)
{
	Store *cfg = store;

	return fgetc(cfg->resource);
}

/**
 * A StoreUnreader function for files
 *
 * @param store		the store to to unread to
 * @param c				the character to unread
 */
API void storeFileUnread(void *store, char c)
{
	Store *cfg = store;

	ungetc(c, cfg->resource);
}

/**
 * A StoreReader function for strings
 *
 * @param store		the store to to read from
 * @result				the read character
 */
API char storeStringRead(void *store)
{
	Store *cfg = store;

	return *((char *) cfg->resource++);
}

/**
 * A StoreUnreader function for strings
 *
 * @param store		the store to to unread to
 * @param c				the character to unread
 */
API void storeStringUnread(void *store, char c)
{
	Store *cfg = store;

	cfg->resource--;
}
