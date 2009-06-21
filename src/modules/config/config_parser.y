/**
 * @file config_parser.y
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
	#include <stdlib.h>
	#include <stdio.h>
	#include <glib.h>
	#include <ctype.h>
	#include "dll.h"
	#include "types.h"
	#include "api.h"
	#include "config.h"
	#include "config_lexer.h"
	static void yyerror(ConfigFile *config, char *error);
%}

%define api.pure
%parse-param {ConfigFile *config}
%lex-param {ConfigFile *config}

%token <string> STRING
%token <integer> INTEGER
%token <float_number> FLOAT_NUMBER

%%
categories:		// empty file
			|	categories '\n' '[' STRING ']' nodes
;

nodes:		'\n' // no node
		|	nodes '\n' node
;

node:	STRING '=' value
;

value:		STRING
		|	INTEGER
		|	FLOAT_NUMBER
		|	'[' list ']'
		|	'{' array '}'
;

list:		value // first element of the list
		|	list ',' value // the list continues
;

array:		node // first element of the array
		|	array ',' node // the array continues
;

%%

static void yyerror(ConfigFile *config, char *error)
{

}
