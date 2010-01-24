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

int yyparse(StoreParser *parser); // this can't go into a header because it doesn't have an API export

/**
 * Parses a store file
 *
 * @param filename		the file name of the store file to parse
 * @result				the parsed store
 */
API Store *parseStoreFile(char *filename)
{
	StoreParser parser;
	parser.resource = fopen(filename, "r");
	parser.read = &storeFileRead;
	parser.unread = &storeFileUnread;
	parser.store = NULL;

	if(parser.resource == NULL) {
		LOG_SYSTEM_ERROR("Could not open store file %s", filename);
		return NULL;
	}

	LOG_INFO("Parsing store file %s", filename);

	if(yyparse(&parser) != 0) {
		LOG_ERROR("Parsing store file %s failed", filename);
		fclose(parser.resource);
		return NULL;
	}

	fclose(parser.resource);

	return parser.store;
}

/**
 * Parses a store string
 *
 * @param string		the store string to parse
 * @result				the parsed store
 */
API Store *parseStoreString(char *string)
{
	StoreParser parser;
	parser.resource = string;
	parser.read = &storeStringRead;
	parser.unread = &storeStringUnread;
	parser.store = NULL;

	LOG_INFO("Parsing store string: %s", string);

	if(yyparse(&parser) != 0) {
		LOG_ERROR("Parsing store string failed");
		return NULL;
	}

	return parser.store;
}

/**
 * A StoreReader function for files
 *
 * @param parser_p		the parser to to read from
 * @result				the read character
 */
API char storeFileRead(void *parser_p)
{
	StoreParser *parser = parser_p;

	return fgetc(parser->resource);
}

/**
 * A StoreUnreader function for files
 *
 * @param parser_p		the parser to to unread to
 * @param c				the character to unread
 */
API void storeFileUnread(void *parser_p, char c)
{
	StoreParser *parser = parser_p;

	ungetc(c, parser->resource);
}

/**
 * A StoreReader function for strings
 *
 * @param parser_p		the parser to to read from
 * @result				the read character
 */
API char storeStringRead(void *parser_p)
{
	StoreParser *parser = parser_p;

	return *((char *) parser->resource++);
}

/**
 * A StoreUnreader function for strings
 *
 * @param parser_p		the parser to to unread to
 * @param c				the character to unread
 */
API void storeStringUnread(void *parser_p, char c)
{
	StoreParser *parser = parser_p;

	parser->resource--;
}