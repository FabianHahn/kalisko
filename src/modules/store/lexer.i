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
 * Lexes a store string and dumps the result
 *
 * @param string		the store string to lex and dump
 * @result				the store's lexer dump as a string, must be freed with g_string_free afterwards
 */
API GString *lexStoreString(char *string);

/**
 * Lexes a store file and dumps the result
 *
 * @param filename		the store file to lex and dump
 * @result				the store's lexer dump as a string, must be freed with g_string_free afterwards
 */
API GString *lexStoreFile(char *filename);

/**
 * Checks if a string can only be expressed in delimited form within a store
 *
 * @param string		the string to test
 * @result				true if the string can only be expressed in delimited form within a store
 */
API bool checkIfStoreStringDelimited(const char *string);

#endif
