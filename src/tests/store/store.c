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

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "dll.h"
#include "test.h"
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/parser.h"
#include "modules/store/lexer.h"
#include "modules/store/path.h"

#include "api.h"

TEST_CASE(lexer);
TEST_CASE(parser);
TEST_CASE(path_modify);
TEST_CASE(path_create);
TEST_CASE(path_split);

static char *lexer_test_input = "  \t \nsomekey = 1337somevalue // comment that is hopefully ignored\nsomeotherkey=\"some\\\\[other \\\"value//}\"\nnumber = 42\nfloat  = 3.14159265";
static int lexer_test_solution_tokens[] = {STRING, '=', STRING, STRING, '=', STRING, STRING, '=', INTEGER, STRING, '=', FLOAT_NUMBER};
static YYSTYPE lexer_test_solution_values[] = {{"somekey"}, {NULL}, {"1337somevalue"}, {"someotherkey"}, {NULL}, {"some\\[other \"value//}"}, {"number"}, {NULL}, {.integer = 42}, {"float"}, {NULL}, {.float_number = 3.14159265}};

static char *parser_test_input = "foo = \"//bar//\" // comment that is hopefully ignored \nsomevalue = (13, 18.34, {bird = word, foo = bar})";

static char *path_test_input = "somekey=(foo bar {foo=bar subarray={bird=word answer=42 emptylist=()}}{}())";

static char *path_split_input = "this/is a \"difficult\"/path\\\\to/split\\/:)";
static char *path_split_solution[] = {"this", "is a \"difficult\"", "path\\to", "split/:)"};

static char _storeStringRead(void *store);
static void _storeStringUnread(void *store, char c);

MODULE_NAME("test_store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the store module");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 2, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 4, 0));

TEST_SUITE_BEGIN(store)
	TEST_CASE_ADD(lexer);
	TEST_CASE_ADD(parser);
	TEST_CASE_ADD(path_modify);
	TEST_CASE_ADD(path_create);
	TEST_CASE_ADD(path_split);
TEST_SUITE_END

TEST_CASE(lexer)
{
	int *solution_tokens;
	YYSTYPE *solution_values;
	int lexx;
	YYSTYPE val;
	YYLTYPE loc;

	StoreParser parser;
	parser.resource = lexer_test_input;
	parser.read = &_storeStringRead;
	parser.unread = &_storeStringUnread;

	solution_tokens = lexer_test_solution_tokens;
	solution_values = lexer_test_solution_values;

	memset(&val, 0, sizeof(YYSTYPE));

	while((lexx = $(int, store, yylex)(&val, &loc, &parser)) != 0) {
		TEST_ASSERT(lexx == *(solution_tokens++));

		switch(lexx) {
			case STRING:
				TEST_ASSERT(strcmp(val.string, solution_values->string) == 0);
				free(val.string);
			break;
			case INTEGER:
				TEST_ASSERT(val.integer == solution_values->integer);
			break;
			case FLOAT_NUMBER:
				TEST_ASSERT(val.float_number == solution_values->float_number);
			break;
		}

		memset(&val, 0, sizeof(YYSTYPE));
		solution_values++;
	}

	TEST_PASS;
}

TEST_CASE(parser)
{
	Store *store;

	TEST_ASSERT((store = $(Store *, store, parseStoreString)(parser_test_input)) != NULL);

	$(void, store, freeStore)(store);

	TEST_PASS;
}

TEST_CASE(path_modify)
{
	Store *value;
	Store *store = $(Store *, store, parseStoreString)(path_test_input);
	TEST_ASSERT(store != NULL);

	// check some path types
	TEST_ASSERT($(Store *, store, getStorePath)(store, "")->type == STORE_ARRAY);
	TEST_ASSERT($(Store *, store, getStorePath)(store, "somekey")->type == STORE_LIST);
	TEST_ASSERT($(Store *, store, getStorePath)(store, "somekey/2")->type == STORE_ARRAY);

	value = $(Store *, store, getStorePath)(store, "somekey/2/subarray/bird");
	TEST_ASSERT(value->type == STORE_STRING);
	TEST_ASSERT(strcmp($(void *, store, getStoreValueContent)(value), "word") == 0);

	// change value
	value = ALLOCATE_OBJECT(Store);
	value->type = STORE_FLOAT_NUMBER;
	value->content.float_number = 13.37;
	TEST_ASSERT($(bool, store, setStorePath)(store, "somekey/2/subarray/bird", value));

	// check if correctly changed
	value = $(Store *, store, getStorePath)(store, "somekey/2/subarray/bird");
	TEST_ASSERT(value->type == STORE_FLOAT_NUMBER);
	TEST_ASSERT(*((double *) $(void *, store, getStoreValueContent)(value)) == 13.37);

	value = $(Store *, store, getStorePath)(store, "somekey/2/subarray/answer");
	TEST_ASSERT(value->type == STORE_INTEGER);
	TEST_ASSERT(*((int *) $(void *, store, getStoreValueContent)(value)) == 42);

	// delete value
	TEST_ASSERT($(bool, store, deleteStorePath)(store, "somekey/2/subarray/answer"));

	// check if correctly deleted
	TEST_ASSERT($(Store *, store, getStorePath)(store, "somekey/2/subarray/answer") == NULL);

	// test list out of bounds handling
	TEST_ASSERT($(Store *, store, getStorePath)(store, "somekey/1337") == NULL);

	$(void, store, freeStore)(store);

	TEST_PASS;
}

TEST_CASE(path_create)
{
	Store *store = $(Store *, store, createStore)();

	TEST_ASSERT($(bool, store, setStorePath)(store, "string", $(Store *, store, createStoreStringValue)("\"e = mc^2\"")));
	TEST_ASSERT($(bool, store, setStorePath)(store, "integer", $(Store *, store, createStoreIntegerValue)(1337)));
	TEST_ASSERT($(bool, store, setStorePath)(store, "float number", $(Store *, store, createStoreFloatNumberValue)(3.141)));
	TEST_ASSERT($(bool, store, setStorePath)(store, "list", $(Store *, store, createStoreListValue)(NULL)));
	TEST_ASSERT($(bool, store, setStorePath)(store, "list/1", $(Store *, store, createStoreStringValue)("the bird is the word")));
	TEST_ASSERT($(bool, store, setStorePath)(store, "array", $(Store *, store, createStoreArrayValue)(NULL)));
	TEST_ASSERT($(bool, store, setStorePath)(store, "array/some\\/sub\\\\array", $(Store *, store, createStoreArrayValue)(NULL)));

	$(void, store, freeStore)(store);

	TEST_PASS;
}

TEST_CASE(path_split)
{
	GPtrArray *array = $(GPtrArray *, store, splitStorePath)(path_split_input);

	for(int i = 0; i < array->len; i++) {
		TEST_ASSERT(strcmp(array->pdata[i], path_split_solution[i]) == 0);
		free(array->pdata[i]);
	}

	g_ptr_array_free(array, TRUE);

	TEST_PASS;
}

/**
 * A local wrapper around storeStringRead
 *
 * @param store		the store to to read from
 * @result				the read character
 */
static char _storeStringRead(void *store)
{
	return $(char, store, storeStringRead)(store);
}

/**
 * A local wrapper around storeStringUnread
 *
 * @param store		the store to to read from
 * @result				the read character
 */
static void _storeStringUnread(void *store, char c)
{
	return $(void, store, storeStringUnread)(store, c);
}