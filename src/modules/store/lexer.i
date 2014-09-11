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

#ifndef STORE_LEXER_H
#define STORE_LEXER_H

#include <glib.h>
#include "store.h"
#include "parse.h"
#include "parser.h"

typedef enum {
	/** Delimiters are the characters that are also grammar elements : '(', ')', '{', '}', '=' */
	STORE_LEX_CHAR_TYPE_DELIMITER,
	/** Spaces separate different grammer elements, but have no grammar meaning of their own */
	STORE_LEX_CHAR_TYPE_SPACE,
	/** The quotation character '"' is used to define extended strings */
	STORE_LEX_CHAR_TYPE_QUOTATION,
	/** The characters '/' and '#' are used to begin comments */
	STORE_LEX_CHAR_TYPE_COMMENT,
	/** The character '\\' is used to escape other characters whithin extended strings */
	STORE_LEX_CHAR_TYPE_ESCAPE,
	/** The decimal mark '.' is used to define floating point numbers */
	STORE_LEX_CHAR_TYPE_DECIMAL,
	/** The characters '\0' and EOF denote the end of the input */
	STORE_LEX_CHAR_TYPE_END,
	/** Digits and '-' are used to define numbers */
	STORE_LEX_CHAR_TYPE_DIGIT,
	/** Everything else is considered a letter in a string */
	STORE_LEX_CHAR_TYPE_LETTER
} StoreLexCharType;

typedef struct {
	int token;
	YYSTYPE value;
} StoreLexResult;

/**
 * Returns the store lexing character type for a char
 *
 * @param c			the int-valued char to analyze
 * @result			the lexing character type of c
 */
API StoreLexCharType getStoreLexCharType(int c);

/**
 * Lexes a token from a store
 *
 * @param lval		the lexer value target
 * @param lloc		the lexer location
 * @param parser	the parser context to lex from
 * @result			the lexed token
 */
API int yylex(YYSTYPE *lval, YYLTYPE *lloc, StoreParser *parser);

/**
 * Lexes a store and returns the sequence of results
 *
 * @param parser		the parser struct supplying the store to lex
 * @result				an array of StoreLexResult objects that must be freed by the caller
 */
API GPtrArray *lexStore(StoreParser *parser);

/**
 * Lexes a store string and returns the sequence of results
 *
 * @param string		the store string to lex and dump
 * @result				an array of StoreLexResult objects that must be freed by the caller
 */
API GPtrArray *lexStoreString(const char *string);

/**
 * Lexes a store file and returns the sequence of results
 *
 * @param filename		the store file to lex and dump
 * @result				an array of StoreLexResult objects that must be freed by the caller
 */
API GPtrArray *lexStoreFile(const char *filename);

/**
 * Dumps the results of a lexing run into a string
 *
 * @param results		a GPtrArray of StoreLexResult objects that should be dumped
 * @result				string with the dumped results, must be freed by the caller
 */
API char *dumpLexResults(GPtrArray *results);

/**
 * Checks if a string can be expressed in simple form within a store
 *
 * @param string		the string to test
 * @result				true if the string can be expressed in simple form
 */
API bool checkSimpleStoreStringCapability(const char *string);

#endif
