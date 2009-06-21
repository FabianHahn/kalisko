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
	static void yyerror(Config *config, char *error);
%}

%define api.pure
%parse-param {Config *config}
%lex-param {Config *config}

%token <string> STRING
%token <integer> INTEGER
%token <float_number> FLOAT_NUMBER
%type <value> array
%type <value> list
%type <value> value
%type <node> node
%type <nodes> nodes
%type <section> section

%%
sections:		section
				{
					config->sections = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &destroyGHashTable);
					g_hash_table_insert(config->sections, $1->name, $1->nodes);
					free($1);
				}
			|	sections section
				{
					g_hash_table_insert(config->sections, $2->name, $2->nodes);
					free($2);
				}
;

section:		'[' STRING ']' newlines // empty section
				{
					$$ = allocateObject(ConfigSection);
					$$->name = strdup($2);
					$$->nodes = g_hash_table_new(&g_str_hash, &g_str_equal);
				}
			|	'[' STRING ']' newlines nodes
				{
					$$ = allocateObject(ConfigSection);
					$$->name = strdup($2);
					$$->nodes = $5;
				}
;

nodes:		node
			{
				$$ = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConfigNodeValue);
				g_hash_table_insert($$, $1->key, $1->value);
				free($1);
			}
		|	nodes node newlines
			{
				g_hash_table_insert($1, $2->key, $2->value);
				free($2);
				$$ = $1;
			}
;

node:	STRING '=' value
		{
			$$ = allocateObject(ConfigNode);
			$$->key = strdup($1);
			$$->value = $3;
		}
;

value:		STRING
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_STRING;
				$$->content.string = strdup($1);
			}
		|	INTEGER
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_INTEGER;
				$$->content.integer = $1;
			}
		|	FLOAT_NUMBER
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_FLOAT_NUMBER;
				$$->content.float_number = $1;
			}
		|	'[' ']'
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_LIST;
				$$->content.list = NULL;
			}
		|	'[' list ']'
			{
				$$ = $2;
			}
		|	'{' '}'
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_ARRAY;
				$$->content.array = g_hash_table_new(&g_str_hash, &g_str_equal);
			}
		|	'{' array '}'
			{
				$$ = $2;
			}
;

list:		value // first element of the list
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_LIST;
				$$->content.list = g_list_append(NULL, $1);
			}
		|	list ',' value // the list continues
			{
				$1->content.list = g_list_append($1->content.list, $3);
				$$ = $1;
			}
;

array:		node // first element of the array
			{
				$$ = allocateObject(ConfigNodeValue);
				$$->type = CONFIG_ARRAY;
				$$->content.array = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConfigNodeValue);
				g_hash_table_insert($$->content.array, $1->key, $1->value);
				free($1);
			}
		|	array ',' node // the array continues
			{
				g_hash_table_insert($1->content.array, $3->key, $3->value);
				free($3);
				$$ = $1;
			}
;

newlines:		'\n'
			|	newlines '\n'
;
%%

static void yyerror(Config *config, char *error)
{
	logError("Syntax error while parsing %s: %s", config->name, error);
}
