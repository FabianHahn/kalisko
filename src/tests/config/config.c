/**
 * @file sample.c
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
#include "modules/config/parse.h"
#include "modules/config/parser.h"
#include "modules/config/lexer.h"

#include "api.h"

TEST_CASE(lexer);
TEST_CASE(parser);

static char *lexer_test_input = "[section]  \t \nsomekey = 1337somevalue // comment that is hopefully ignored\nsomeotherkey=\"some\\\\[other \\\"value//}\"\nnumber = 42\nfloat  = 3.14159265";
static int lexer_test_solution_tokens[] = {'[', STRING, ']', '[', STRING, ']', STRING, '=', STRING, STRING, '=', STRING, STRING, '=', INTEGER, STRING, '=', FLOAT_NUMBER};
static YYSTYPE lexer_test_solution_values[] = {{NULL}, {"default"}, {NULL}, {NULL}, {"section"}, {NULL}, {"somekey"}, {NULL}, {"1337somevalue"}, {"someotherkey"}, {NULL}, {"some\\[other \"value//}"}, {"number"}, {NULL}, {.integer = 42}, {"float"}, {NULL}, {.float_number = 3.14159265}};

static char *parser_test_input = "[firstsection]\n\n[section]foo = \"//bar//\" // comment that is hopefully ignored \nsomevalue = [13, 18.34, {bird = word, foo = bar}]";

static char _configStringRead(void *config);
static void _configStringUnread(void *config, char c);

TEST_SUITE_BEGIN(config)
	TEST_CASE_ADD(lexer);
	TEST_CASE_ADD(parser);
TEST_SUITE_END

TEST_CASE(lexer)
{
	int *solution_tokens;
	YYSTYPE *solution_values;
	int lexx;
	YYSTYPE val;
	YYLTYPE loc;

	Config *config = allocateObject(Config);

	config->name = lexer_test_input;
	config->resource = config->name;
	config->read = &_configStringRead;
	config->unread = &_configStringUnread;

	solution_tokens = lexer_test_solution_tokens;
	solution_values = lexer_test_solution_values;

	memset(&val, 0, sizeof(YYSTYPE));

	while((lexx = yylex(&val, &loc, config)) != 0) {
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

	free(config);

	TEST_PASS;
}

TEST_CASE(parser)
{
	Config *config;

	TEST_ASSERT((config = parseConfigString(parser_test_input)) != NULL);

	freeConfig(config);

	TEST_PASS;
}

API GList *module_depends()
{
	return g_list_append(NULL, "config");
}

/**
 * A local wrapper around configStringRead
 *
 * @param config		the config to to read from
 * @result				the read character
 */
static char _configStringRead(void *config)
{
	return configStringRead(config);
}

/**
 * A local wrapper around configStringUnread
 *
 * @param config		the config to to read from
 * @result				the read character
 */
static void _configStringUnread(void *config, char c)
{
	return configStringUnread(config, c);
}
