/**
 * @file parser.y
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

%{
	#include <stdio.h>
	#include <string.h>
	#include <glib.h>
	#include "dll.h"
	#include "types.h"
	#include "log.h"
	#include "memory_alloc.h"
	#include "util.h"
	#include "api.h"
	#include "parse.h"
	#include "lexer.h"
/*
	#define YYDEBUG 1
	#define YYFPRINTF fprintf
	int yydebug = 1;
*/
%}

%locations
%define api.pure
%parse-param {Config *config}
%lex-param {Config *config}

%{
	void yyerror(YYLTYPE *lloc, Config *config, char *error);
%}

%token <string> STRING
%token <integer> INTEGER
%token <float_number> FLOAT_NUMBER
%type <value> array
%type <value> list
%type <value> value
%type <node> node
%type <nodes> nodes
%type <section> section
%destructor { freeConfigNodeValue($$); } array list value
%destructor { freeConfigNodeValue($$->value); free($$->key); free($$); } node
%destructor { g_hash_table_destroy($$); } nodes
%destructor { free($$->name); g_hash_table_destroy($$->nodes); free($$); } section

%%
sections:		section
				{
					@$.last_line = @1.last_line;
					@$.last_column = @1.last_column;
					config->sections = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &destroyGHashTable);
					g_hash_table_insert(config->sections, $1->name, $1->nodes);
					free($1);
				}
			|	sections section
				{
					@$.last_line = @1.last_line;
					@$.last_column = @1.last_column;
					g_hash_table_insert(config->sections, $2->name, $2->nodes);
					free($2);
				}
;

section:		'[' STRING ']' // empty section
				{
					@$.last_line = @1.last_line;
					@$.last_column = @1.last_column;
					$$ = allocateObject(ConfigSection);
					$$->name = strdup($2);
					$$->nodes = g_hash_table_new(&g_str_hash, &g_str_equal);
				}
			|	'[' STRING ']' nodes
				{
					@$.last_line = @1.last_line;
					@$.last_column = @1.last_column;
					$$ = allocateObject(ConfigSection);
					$$->name = strdup($2);
					$$->nodes = $4;
				}
;

nodes:		node
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConfigNodeValue);
				g_hash_table_insert($$, $1->key, $1->value);
				free($1);
			}
		|	nodes node
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				g_hash_table_insert($1, $2->key, $2->value);
				free($2);
				$$ = $1;
			}
;

node:	STRING '=' value
		{
			@$.last_line = @1.last_line;
			@$.last_column = @1.last_column;
			$$ = allocateObject(ConfigNode);
			$$->key = strdup($1);
			$$->value = $3;
		}
;

value:		STRING
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_STRING;
				$$->content.string = strdup($1);
			}
		|	INTEGER
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_INTEGER;
				$$->content.integer = $1;
			}
		|	FLOAT_NUMBER
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_FLOAT_NUMBER;
				$$->content.float_number = $1;
			}
		|	'(' ')'
			{
				@$.last_line = @2.last_line;
				@$.last_column = @2.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_LIST;
				$$->content.list = NULL;
			}
		|	'(' list ')'
			{
				@$.last_line = @3.last_line;
				@$.last_column = @3.last_column;
				$$ = $2;
			}
		|	'{' '}'
			{
				@$.last_line = @2.last_line;
				@$.last_column = @2.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_ARRAY;
				$$->content.array = g_hash_table_new(&g_str_hash, &g_str_equal);
			}
		|	'{' array '}'
			{
				@$.last_line = @3.last_line;
				@$.last_column = @3.last_column;
				$$ = $2;
			}
;

list:		value // first element of the list
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_LIST;
				$$->content.list = g_queue_new();
				g_queue_push_head($$->content.list, $1);
			}
		|	list value // the list continues
			{
				@$.last_line = @2.last_line;
				@$.last_column = @2.last_column;
				g_queue_push_tail($1->content.list, $2);
				$$ = $1;
			}
;

array:		node // first element of the array
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_ARRAY;
				$$->content.array = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConfigNodeValue);
				g_hash_table_insert($$->content.array, $1->key, $1->value);
				free($1);
			}
		|	array node // the array continues
			{
				@$.last_line = @2.last_line;
				@$.last_column = @2.last_column;
				g_hash_table_insert($1->content.array, $2->key, $2->value);
				free($2);
				$$ = $1;
			}
;
%%

void yyerror(YYLTYPE *lloc, Config *config, char *error)
{
	logError("Parse error in %s at line %d, column %d: %s", config->name, lloc->last_line, lloc->last_column - 1, error);
}
