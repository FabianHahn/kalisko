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
	#include "store.h"
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
%parse-param {Store *store}
%lex-param {Store *store}

%{
	void yyerror(YYLTYPE *lloc, Store *store, char *error);
%}

%token <string> STRING
%token <integer> INTEGER
%token <float_number> FLOAT_NUMBER
%type <value> array
%type <value> list
%type <value> value
%type <node> node
%destructor { freeStoreNodeValue($$); } array list value
%destructor { freeStoreNodeValue($$->value); free($$->key); free($$); } node

%%
root:		// empty string
			{
				store->root = createStoreArrayValue(NULL);
			}
		|	array
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				store->root = $1;
			}
;

node:	STRING '=' value
		{
			@$.last_line = @1.last_line;
			@$.last_column = @1.last_column;
			$$ = ALLOCATE_OBJECT(StoreNode);
			$$->key = strdup($1);
			$$->value = $3;
		}
;

value:		STRING
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = createStoreStringValue($1);
			}
		|	INTEGER
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = createStoreIntegerValue($1);
			}
		|	FLOAT_NUMBER
			{
				@$.last_line = @1.last_line;
				@$.last_column = @1.last_column;
				$$ = createStoreFloatNumberValue($1);
			}
		|	'(' ')'
			{
				@$.last_line = @2.last_line;
				@$.last_column = @2.last_column;
				$$ = createStoreListValue(NULL);
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
				$$ = createStoreArrayValue(NULL);
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
				$$ = createStoreListValue(NULL);
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
				$$ = createStoreArrayValue(NULL);
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

void yyerror(YYLTYPE *lloc, Store *store, char *error)
{
	LOG_ERROR("Parse error in %s at line %d, column %d: %s", store->name, lloc->last_line, lloc->last_column - 1, error);
}
