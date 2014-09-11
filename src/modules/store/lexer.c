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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "dll.h"
#include "types.h"
#include "memory_alloc.h"
#include "log.h"

#define API
#include "parse.h"
#include "parser.h"
#include "lexer.h"

typedef enum {
	LEXER_STATE_START,
	LEXER_STATE_COMMENTING,
	LEXER_STATE_READING_STRING_SIMPLE,
	LEXER_STATE_READING_STRING_EXTENDED,
	LEXER_STATE_READING_STRING_EXTENDED_ESCAPING,
	LEXER_STATE_READING_NUMBER_INT,
	LEXER_STATE_READING_NUMBER_FLOAT
} LexerState;

typedef struct {
	int c;
	StoreLexCharType type;
	LexerState state;
	GString *assemble;
	int token;
} LexerStruct;

typedef enum {
	LEXER_ACTION_REPEAT,
	LEXER_ACTION_CONSUME,
	LEXER_ACTION_SKIP,
	LEXER_ACTION_RETURN,
	LEXER_ACTION_PUSHBACK_RETURN
} LexerAction;

static LexerAction lexStateStart(LexerStruct *lex);
static LexerAction lexStateCommenting(LexerStruct *lex);
static LexerAction lexStateReadingStringSimple(LexerStruct *lex);
static LexerAction lexStateReadingStringExtended(LexerStruct *lex);
static LexerAction lexStateReadingStringExtendedEscaping(LexerStruct *lex);
static LexerAction lexStateReadingNumberInt(LexerStruct *lex);
static LexerAction lexStateReadingNumberFloat(LexerStruct *lex);
static void freeStoreLexResult(void *result_p);

void yyerror(YYLTYPE *lloc, StoreParser *parser, char *error); // this can't go into a header because it doesn't have an API export

API StoreLexCharType getStoreLexCharType(int c)
{
	if(isspace(c)) {
		return STORE_LEX_CHAR_TYPE_SPACE;
	}

	if(isdigit(c)) {
		return STORE_LEX_CHAR_TYPE_DIGIT;
	}

	switch(c) {
		case EOF:
		case '\0':
			return STORE_LEX_CHAR_TYPE_END;
		case '.':
			return STORE_LEX_CHAR_TYPE_DECIMAL;
		case '"':
			return STORE_LEX_CHAR_TYPE_QUOTATION;
		case '\\':
			return STORE_LEX_CHAR_TYPE_ESCAPE;
		case '/':
		case '#':
			return STORE_LEX_CHAR_TYPE_COMMENT;
		case '(':
		case ')':
		case '{':
		case '}':
		case '=':
			return STORE_LEX_CHAR_TYPE_DELIMITER;
		case ';':
		case ',':
			return STORE_LEX_CHAR_TYPE_SPACE;
		case '-':
			return STORE_LEX_CHAR_TYPE_DIGIT;
		default:
			return STORE_LEX_CHAR_TYPE_LETTER;
	}
}

API int yylex(YYSTYPE *lval, YYLTYPE *lloc, StoreParser *parser)
{
	LexerStruct lex;
	lex.c = EOF;
	lex.type = STORE_LEX_CHAR_TYPE_END;
	lex.state = LEXER_STATE_START;
	lex.assemble = g_string_new("");
	lex.token = 0;

	while(true) {
		lex.c = parser->read(parser);
		lex.type = getStoreLexCharType(lex.c);

		lloc->last_column++;

		if(lex.c == '\n') {
			lloc->last_line++;
			lloc->last_column = 1;
		}

		LexerAction action = LEXER_ACTION_RETURN;

		switch(lex.state) {
			case LEXER_STATE_START:
				action = lexStateStart(&lex);
				break;
			case LEXER_STATE_COMMENTING:
				action = lexStateCommenting(&lex);
				break;
			case LEXER_STATE_READING_STRING_SIMPLE:
				action = lexStateReadingStringSimple(&lex);
				break;
			case LEXER_STATE_READING_STRING_EXTENDED:
				action = lexStateReadingStringExtended(&lex);
				break;
			case LEXER_STATE_READING_STRING_EXTENDED_ESCAPING:
				action = lexStateReadingStringExtendedEscaping(&lex);
				break;
			case LEXER_STATE_READING_NUMBER_INT:
				action = lexStateReadingNumberInt(&lex);
				break;
			case LEXER_STATE_READING_NUMBER_FLOAT:
				action = lexStateReadingNumberFloat(&lex);
				break;
		}

		switch(action) {
			case LEXER_ACTION_REPEAT:
				parser->unread(parser, lex.c);
				lloc->last_column--;
				break;
			case LEXER_ACTION_CONSUME:
				g_string_append_c(lex.assemble, lex.c);
				break;
			case LEXER_ACTION_SKIP:
				break;
			case LEXER_ACTION_RETURN:
			case LEXER_ACTION_PUSHBACK_RETURN:
			{
				if(action == LEXER_ACTION_PUSHBACK_RETURN) {
					parser->unread(parser, lex.c);
					lloc->last_column--;
				}

				bool freeAssemble = true;

				switch(lex.token) {
					case -1:
						yyerror(lloc, parser, lex.assemble->str);
					case STORE_TOKEN_STRING:
						freeAssemble = false;
						lval->string = lex.assemble->str;
						break;
					case STORE_TOKEN_INTEGER:
						lval->integer = atoi(lex.assemble->str);
						break;
					case STORE_TOKEN_FLOAT_NUMBER:
						lval->float_number = atof(lex.assemble->str);
						break;
					default:
						// nothing to do
						break;
				}

				g_string_free(lex.assemble, freeAssemble);
				return lex.token;
			} break;
		}
	}

	assert(false); // this should never be reached
	return 0;
}

static LexerAction lexStateStart(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_DELIMITER:
			lex->token = lex->c;
			return LEXER_ACTION_RETURN;
		case STORE_LEX_CHAR_TYPE_SPACE:
			return LEXER_ACTION_SKIP;
		case STORE_LEX_CHAR_TYPE_QUOTATION:
			lex->state = LEXER_STATE_READING_STRING_EXTENDED;
			return LEXER_ACTION_SKIP;
		case STORE_LEX_CHAR_TYPE_COMMENT:
			lex->state = LEXER_STATE_COMMENTING;
			return LEXER_ACTION_SKIP;
		case STORE_LEX_CHAR_TYPE_ESCAPE:
			lex->state = LEXER_STATE_READING_STRING_SIMPLE;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DECIMAL:
			lex->state = LEXER_STATE_READING_NUMBER_FLOAT;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_END:
			return LEXER_ACTION_RETURN;
		case STORE_LEX_CHAR_TYPE_DIGIT:
			lex->state = LEXER_STATE_READING_NUMBER_INT;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_LETTER:
			lex->state = LEXER_STATE_READING_STRING_SIMPLE;
			return LEXER_ACTION_CONSUME;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateCommenting(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_QUOTATION:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_ESCAPE:
		case STORE_LEX_CHAR_TYPE_DECIMAL:
		case STORE_LEX_CHAR_TYPE_DIGIT:
		case STORE_LEX_CHAR_TYPE_LETTER:
			if(lex->c == '\n') {
				lex->state = LEXER_STATE_START;
			}
			return LEXER_ACTION_SKIP;
		case STORE_LEX_CHAR_TYPE_END:
			return LEXER_ACTION_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateReadingStringSimple(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_QUOTATION:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_END:
			lex->token = STORE_TOKEN_STRING;
			return LEXER_ACTION_PUSHBACK_RETURN;
		case STORE_LEX_CHAR_TYPE_ESCAPE:
		case STORE_LEX_CHAR_TYPE_DECIMAL:
		case STORE_LEX_CHAR_TYPE_DIGIT:
		case STORE_LEX_CHAR_TYPE_LETTER:
			return LEXER_ACTION_CONSUME;
			return LEXER_ACTION_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateReadingStringExtended(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_QUOTATION:
			lex->token = STORE_TOKEN_STRING;
			return LEXER_ACTION_RETURN;
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_DECIMAL:
		case STORE_LEX_CHAR_TYPE_DIGIT:
		case STORE_LEX_CHAR_TYPE_LETTER:
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_ESCAPE:
			lex->state = LEXER_STATE_READING_STRING_EXTENDED_ESCAPING;
			return LEXER_ACTION_SKIP;
		case STORE_LEX_CHAR_TYPE_END:
			lex->token = -1;
			g_string_assign(lex->assemble, "Unexpected end when reading extended string");
			return LEXER_ACTION_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateReadingStringExtendedEscaping(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_QUOTATION:
		case STORE_LEX_CHAR_TYPE_ESCAPE:
			lex->state = LEXER_STATE_READING_STRING_EXTENDED;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_DECIMAL:
		case STORE_LEX_CHAR_TYPE_END:
		case STORE_LEX_CHAR_TYPE_DIGIT:
		case STORE_LEX_CHAR_TYPE_LETTER:
			lex->token = -1;
			g_string_assign(lex->assemble, "Unexpected escape character without following character to be escaped when reading extended string");
			return LEXER_ACTION_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateReadingNumberInt(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_DIGIT:
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DECIMAL:
			// actually reading a float, we just didn't know yet
			lex->state = LEXER_STATE_READING_NUMBER_FLOAT;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_LETTER:
			// actually reading a simple string, we just didn't know yet
			lex->state = LEXER_STATE_READING_STRING_SIMPLE;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_QUOTATION:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_ESCAPE:
		case STORE_LEX_CHAR_TYPE_END:
			lex->token = STORE_TOKEN_INTEGER;
			return LEXER_ACTION_PUSHBACK_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static LexerAction lexStateReadingNumberFloat(LexerStruct *lex)
{
	switch(lex->type) {
		case STORE_LEX_CHAR_TYPE_DIGIT:
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DECIMAL:
			lex->token = -1;
			g_string_assign(lex->assemble, "Encountered double decimal mark when reading float number");
			return LEXER_ACTION_RETURN;
		case STORE_LEX_CHAR_TYPE_LETTER:
			// actually reading a simple string, we just didn't know yet
			lex->state = LEXER_STATE_READING_STRING_SIMPLE;
			return LEXER_ACTION_CONSUME;
		case STORE_LEX_CHAR_TYPE_DELIMITER:
		case STORE_LEX_CHAR_TYPE_SPACE:
		case STORE_LEX_CHAR_TYPE_QUOTATION:
		case STORE_LEX_CHAR_TYPE_COMMENT:
		case STORE_LEX_CHAR_TYPE_ESCAPE:
		case STORE_LEX_CHAR_TYPE_END:
			lex->token = STORE_TOKEN_FLOAT_NUMBER;
			return LEXER_ACTION_PUSHBACK_RETURN;
	}

	assert(false); // this should never be reached
	return LEXER_ACTION_RETURN;
}

static void freeStoreLexResult(void *result_p)
{
	StoreLexResult *result = (StoreLexResult *) result_p;

	if(result->token == STORE_TOKEN_STRING) {
		free(result->value.string);
	}

	free(result);
}

API GPtrArray *lexStore(StoreParser *parser)
{
	GPtrArray *results = g_ptr_array_new_with_free_func(&freeStoreLexResult);

	YYSTYPE value;
	YYLTYPE loc;
	int token;

	while((token = yylex(&value, &loc, parser)) != 0) {
		StoreLexResult *result = ALLOCATE_OBJECT(StoreLexResult);
		result->token = token;
		result->value = value;

		g_ptr_array_add(results, result);
	}

	return results;
}

API GPtrArray *lexStoreString(const char *string)
{
	StoreParser parser;
	parser.resource = (char *) string;
	parser.read = &storeStringRead;
	parser.unread = &storeStringUnread;

	return lexStore(&parser);
}

API GPtrArray *lexStoreFile(const char *filename)
{
	StoreParser parser;
	parser.resource = fopen(filename, "r");
	parser.read = &storeFileRead;
	parser.unread = &storeFileUnread;

	GPtrArray *ret = lexStore(&parser);

	fclose(parser.resource);

	return ret;
}

API bool checkSimpleStoreStringCapability(const char *string)
{
	GPtrArray *results = lexStoreString(string);

	if(results->len != 1) {
		g_ptr_array_free(results, true);
		return false;
	}

	StoreLexResult *result = (StoreLexResult *) results->pdata[0];

	if(result->token != STORE_TOKEN_STRING) {
		g_ptr_array_free(results, true);
		return false;
	}

	if(g_strcmp0(result->value.string, string) != 0) {
		g_ptr_array_free(results, true);
		return false;
	}

	g_ptr_array_free(results, true);
	return true;
}

API char *dumpLexResults(GPtrArray *results)
{
	GString *ret = g_string_new("");

	for(int i = 0; i < results->len; i++) {
		StoreLexResult *result = (StoreLexResult *) results->pdata[i];

		switch(result->token) {
			case STORE_TOKEN_STRING:
				g_string_append_printf(ret, "<string=\"%s\"> ", result->value.string);
			break;
			case STORE_TOKEN_INTEGER:
				g_string_append_printf(ret, "<integer=%d> ", result->value.integer);
			break;
			case STORE_TOKEN_FLOAT_NUMBER:
				g_string_append_printf(ret, "<float=%f> ", result->value.float_number);
			break;
			default:
				g_string_append_printf(ret, "'%c' ", result->token);
			break;
		}
	}

	char *dump = ret->str;
	g_string_free(ret, false);

	return dump;
}
