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
#include <string.h>

#include "dll.h"
#include "test.h"
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/parser.h"
#include "modules/store/lexer.h"
#include "modules/store/path.h"
#include "modules/store/clone.h"
#include "modules/store/write.h"
#include "modules/store/merge.h"
#include "modules/store/schema.h"
#include "modules/store/validate.h"
#define API

TEST(lexer);
TEST(parser_longstring);
TEST(parser_clone_dump);
TEST(path_modify);
TEST(path_create);
TEST(path_split);
TEST(merge);
TEST(schema_parse);
TEST(schema_selfvalidation);
TEST(schema_crossvalidation);

static char *lexer_test_input = "  \t \nsomekey = 1337somevalue // comment that is hopefully ignored\nsomeotherkey = \"some\\\\[other \\\"value//}\"\nnumber = -42\nfloat  = -3.14159265";
static int lexer_test_solution_tokens[] = {STORE_TOKEN_STRING, '=', STORE_TOKEN_STRING, STORE_TOKEN_STRING, '=', STORE_TOKEN_STRING, STORE_TOKEN_STRING, '=', STORE_TOKEN_INTEGER, STORE_TOKEN_STRING, '=', STORE_TOKEN_FLOAT_NUMBER};
static YYSTYPE lexer_test_solution_values[] = {{"somekey"}, {NULL}, {"1337somevalue"}, {"someotherkey"}, {NULL}, {"some\\[other \"value//}"}, {"number"}, {NULL}, {.integer = -42}, {"float"}, {NULL}, {.float_number = -3.14159265}};

static char *parser_test_input = "foo = \"//bar//\" // comment that is hopefully ignored \nsomevalue = (13, 18.34, {bird = word, foo = bar})";

static char *path_test_input = "somekey=(foo bar {foo=bar subarray={bird=word answer=42 emptylist=()}}{}())";

static char *path_split_input = "this/is a \"difficult\"/path\\\\to/split\\/:)";
static char *path_split_solution[] = {"this", "is a \"difficult\"", "path\\to", "split/:)"};

MODULE_NAME("test_store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the store module");
MODULE_VERSION(0, 8, 1);
MODULE_BCVERSION(0, 8, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 16, 0));

TEST_SUITE_BEGIN(store)
	ADD_SIMPLE_TEST(lexer);
	ADD_SIMPLE_TEST(parser_longstring);
	ADD_SIMPLE_TEST(parser_clone_dump);
	ADD_SIMPLE_TEST(path_modify);
	ADD_SIMPLE_TEST(path_create);
	ADD_SIMPLE_TEST(path_split);
	ADD_SIMPLE_TEST(merge);
	ADD_SIMPLE_TEST(schema_parse);
	ADD_SIMPLE_TEST(schema_selfvalidation);
	ADD_SIMPLE_TEST(schema_crossvalidation);
TEST_SUITE_END

TEST(lexer)
{
	int n = sizeof(lexer_test_solution_tokens) / sizeof(lexer_test_solution_tokens[0]);

	GPtrArray *results = lexStoreString(lexer_test_input);

	char *resultsDump = dumpLexResults(results);
	logNotice("lexer test case dump: %s", resultsDump);
	free(resultsDump);

	TEST_ASSERT(results->len == n);

	for(int i = 0; i < n; i++) {
		StoreLexResult *result = (StoreLexResult *) results->pdata[i];

		TEST_ASSERT(result->token == lexer_test_solution_tokens[i]);

		switch(result->token) {
			case STORE_TOKEN_STRING:
				logNotice("testing string: %s vs %s", result->value.string, lexer_test_solution_values[i].string);
				TEST_ASSERT(g_strcmp0(result->value.string, lexer_test_solution_values[i].string) == 0);
			break;
			case STORE_TOKEN_INTEGER:
				logNotice("testing int: %d vs %d", result->value.integer, lexer_test_solution_values[i].integer);
				TEST_ASSERT(result->value.integer == lexer_test_solution_values[i].integer);
			break;
			case STORE_TOKEN_FLOAT_NUMBER:
				logNotice("testing float: %f vs %f", result->value.float_number, lexer_test_solution_values[i].float_number);
				TEST_ASSERT(result->value.float_number == lexer_test_solution_values[i].float_number);
			break;
		}
	}

	g_ptr_array_free(results, true);
}

TEST(parser_longstring)
{
	GString *input = g_string_new("longstring = ");

	for(int i = 0; i < 10000; i++) {
		g_string_append_c(input, 'a');
	}

	Store *store = parseStoreString(input->str);

	TEST_ASSERT(store != NULL);

	Store *longstring = g_hash_table_lookup(store->content.array, "longstring");

	TEST_ASSERT(longstring->type == STORE_STRING);

	freeStore(store);
	g_string_free(input, true);
}

TEST(parser_clone_dump)
{
	Store *store;
	GString *storeDump;
	Store *clone;
	GString *cloneDump;

	TEST_ASSERT((store = $(Store *, store, parseStoreString)(parser_test_input)) != NULL);
	TEST_ASSERT((storeDump = $(GString *, store, writeStoreGString)(store)) != NULL);
	TEST_ASSERT((clone = $(Store *, store, cloneStore)(store)) != NULL);
	$(void, store, freeStore)(store);
	TEST_ASSERT((cloneDump = $(GString *, store, writeStoreGString)(clone)) != NULL);
	$(void, store, freeStore)(clone);

	TEST_ASSERT(g_strcmp0(storeDump->str, cloneDump->str) == 0);
	g_string_free(storeDump, true);
	g_string_free(cloneDump, true);
}

TEST(path_modify)
{
	Store *value;
	Store *store = $(Store *, store, parseStoreString)(path_test_input);
	TEST_ASSERT(store != NULL);

	// check some path types
	TEST_ASSERT($(Store *, store, getStorePath)(store, "%s", "")->type == STORE_ARRAY);
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
}

TEST(path_create)
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
}

TEST(path_split)
{
	GPtrArray *array = $(GPtrArray *, store, splitStorePath)(path_split_input);

	for(int i = 0; i < array->len; i++) {
		TEST_ASSERT(strcmp(array->pdata[i], path_split_solution[i]) == 0);
		free(array->pdata[i]);
	}

	g_ptr_array_free(array, TRUE);
}

TEST(merge)
{
	Store *store = $(Store *, store, parseStoreString)("replaced = 13; listmerged = (1 2); recursive = { first = beginning }");
	Store *import = $(Store *, store, parseStoreString)("replaced = 3.14159; listmerged = (3); recursive = { last = end }");
	Store *solution = $(Store *, store, parseStoreString)("replaced = 3.14159; listmerged = (1 2 3); recursive = { first = beginning; last = end }");

	TEST_ASSERT($(bool, store, mergeStore)(store, import));
	$(void, store, freeStore)(import);

	GString *mergestr = $(GString *, store, writeStoreGString)(store);
	GString *solutionstr = $(GString *, store, writeStoreGString)(solution);

	TEST_ASSERT(g_strcmp0(mergestr->str, solutionstr->str) == 0);

	g_string_free(mergestr, true);
	g_string_free(solutionstr, true);
	$(void, store, freeStore)(store);
	$(void, store, freeStore)(solution);
}

TEST(schema_parse)
{
	char *execpath = getExecutablePath();
	GString *schemapath = g_string_new(execpath);
	free(execpath);

	g_string_append(schemapath, "/tests/store/test_schema.store");

	Store *schemaStore = parseStoreFile(schemapath->str);

	g_string_free(schemapath, true);

	TEST_ASSERT(schemaStore != NULL);

	Schema *schema = parseSchema(schemaStore);

	TEST_ASSERT(schema != NULL);

	freeSchema(schema);
	freeStore(schemaStore);
}

TEST(schema_selfvalidation)
{
	char *execpath = getExecutablePath();
	GString *schemapath = g_string_new(execpath);
	free(execpath);

	g_string_append(schemapath, "/tests/store/selfvalidation_schema.store");

	Store *schemaStore = parseStoreFile(schemapath->str);

	g_string_free(schemapath, true);

	TEST_ASSERT(schemaStore != NULL);

	Schema *schema = parseSchema(schemaStore);

	TEST_ASSERT(schema != NULL);

	TEST_ASSERT(validateStore(schemaStore, schema));

	freeSchema(schema);
	freeStore(schemaStore);
}

TEST(schema_crossvalidation)
{
	char *execpath = getExecutablePath();

	GString *schemapath = g_string_new(execpath);
	g_string_append(schemapath, "/tests/store/selfvalidation_schema.store");
	Store *schemaStore = parseStoreFile(schemapath->str);
	g_string_free(schemapath, true);

	TEST_ASSERT(schemaStore != NULL);

	GString *testpath = g_string_new(execpath);
	g_string_append(testpath, "/tests/store/test_schema.store");
	Store *testStore = parseStoreFile(testpath->str);
	g_string_free(testpath, true);

	TEST_ASSERT(testStore != NULL);

	free(execpath);

	TEST_ASSERT(validateStoreByStoreSchema(testStore, schemaStore));
	TEST_ASSERT(!validateStoreByStoreSchema(schemaStore, testStore));

	freeStore(schemaStore);
	freeStore(testStore);
}
