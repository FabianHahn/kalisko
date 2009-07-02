/**
 * @file lexer.c
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

#include "api.h"
#include "parse.h"
#include "parser.h"
#include "lexer.h"

/**
 * Is the char a delimiter?
 *
 * @param C		the char to check
 * @result		true if the char is a delimiter
 */
#define IS_DELIMITER(C) (isspace(c) || c == ';' || c == ',')

static GString *dumpLex(Config *config) G_GNUC_WARN_UNUSED_RESULT;

void yyerror(YYLTYPE *lloc, Config *config, char *error); // this can't go into a header because it doesn't have an API export

/**
 * Lexes a token from a config
 *
 * @param lval		the lexer value target
 * @param config	the config to lex
 * @result			the lexed token
 */
API int yylex(YYSTYPE *lval, YYLTYPE *lloc, Config *config)
{
	int c;
	bool escaping = false;
	bool reading_string = false;
	bool string_is_delimited = false;
	bool reading_numeric = false;
	bool numeric_is_float = false;
	int commenting = 0;
	char assemble[CONFIG_MAX_STRING_LENGTH];
	int i = 0;

	if(config->prelude < 3) {
		config->prelude++;

		switch(config->prelude) {
			case 1:
				return '[';
			break;
			case 2:
				lval->string = strdup("default");
				return STRING;
			break;
			case 3:
				return ']';
			break;
		}
	}

	while(true) {
		c = config->read(config);
		lloc->last_column++;

		if(c == '\n') {
			lloc->last_line++;
			lloc->last_column = 1;
			commenting = 0;
		} else if(commenting >= 2) {
			if(c != '\0' || c != EOF) {
				continue; // we're inside a comment
			}
		}

		switch(c) {
			case EOF:
			case '\0':
			case '[':
			case ']':
			case '{':
			case '}':
			case '(':
			case ')':
			case '=':
				if(reading_string) {
					if(!string_is_delimited) { // end of undelimited string reached
						assemble[i] = '\0';
						lval->string = strdup(assemble);
						if(c != EOF) {
							config->unread(config, c); // push back to stream
							lloc->last_column--;
						}
						return STRING;
					} // else just continue reading
				} else if(reading_numeric) {
					if(numeric_is_float) { // end of float reached
						assemble[i] = '\0';
						lval->float_number = atof(assemble);
						if(c != EOF) {
							config->unread(config, c); // push back to stream
							lloc->last_column--;
						}
						return FLOAT_NUMBER;
					} else { // end of int reached
						assemble[i] = '\0';
						lval->integer = atoi(assemble);
						if(c != EOF) {
							config->unread(config, c); // push back to stream
							lloc->last_column--;
						}
						return INTEGER;
					}
				} else {
					if(c == EOF) {
						return 0;
					} else {
						return c;
					}
				}
			break;
		}

		// Any used non-value token will arrive here iff we're reading a delimited string

		if(reading_string) {
			if(string_is_delimited) {
				if(c == '"') {
					if(escaping) { // delimiter is escaped
						escaping = false;
					} else { // end of delimited string reached
						assemble[i] = '\0';
						lval->string = strdup(assemble);
						return STRING;
					}
				} else if(c == '\\') { // escape character
					if(escaping) { // escape character is escaped itself
						escaping = false;
					} else { // starting to escape
						escaping = true;
						continue;
					}
				} // else just continue reading
			} else {
				if(c == '"' || c == '\\') { // delimiter or escape character not allowed in non-delimited string
					yyerror(lloc, config, "Delimiter '\"' or escape character '\\' not allowed in non-delimited string");
					return 0; // error
				} else if(IS_DELIMITER(c)) { // end of non delimited string reached
					assemble[i] = '\0';
					lval->string = strdup(assemble);
					return STRING;
				}
			}
		} else if(reading_numeric) {
			if(c == '.') { // delimiter
				if(numeric_is_float) {
					yyerror(lloc, config, "Multiple occurences of delimiter '.' in numeric value");
					return 0; // error
				} else {
					numeric_is_float = true;
				}
			} else if(IS_DELIMITER(c)) { // whitespace, end of number
				if(numeric_is_float) {
					assemble[i] = '\0';
					lval->float_number = atof(assemble);
					return FLOAT_NUMBER;
				} else {
					assemble[i] = '\0';
					lval->integer = atoi(assemble);
					return INTEGER;
				}
			} else if(!isdigit(c)) { // we're actually reading a non-delimited string
				reading_numeric = false;
				reading_string = true;
				config->unread(config, c); // push back to stream
				lloc->last_column--;
				continue;
			}
		} else { // We're starting to read now!
			if(c == '/') {
				commenting++;
				continue;
			} else {
				commenting = 0;
			}

			if(IS_DELIMITER(c)) { // whitespace
				continue; // just ignore it
			} else if(isdigit(c)) { // reading an int or a float number
				reading_numeric = true;
			} else { // reading a string
				reading_string = true;

				if(c == '"') { // starting a delimited string
					string_is_delimited = true;
					continue;
				} else if(c == '\\') {
					yyerror(lloc, config, "Escape character '\\' not allowed in non-delimited string");
					return 0; // error
				} // else just continue reading the non-delimited string
			}
		}

		if(escaping) { // escape character wasn't used
			yyerror(lloc, config, "Unused escape character '\\'");
			return 0; // error
		}

		assemble[i++] = c;

		if(i >= CONFIG_MAX_STRING_LENGTH) {
			yyerror(lloc, config, "String value exceeded maximum length");
			return 0; // error
		}
	}
}

/**
 * Lexes a config string and dumps the result
 *
 * @param config		the config string to lex and dump
 * @result				the config's lexer dump as a string, must be freed with g_string_free afterwards
 */
API GString *lexConfigString(char *string)
{
	Config *config = allocateObject(Config);

	config->name = string;
	config->resource = config->name;
	config->read = &configStringRead;
	config->unread = &configStringUnread;
	config->prelude = 0;

	GString *ret = dumpLex(config);

	free(config);

	return ret;
}

/**
 * Lexes a config file and dumps the result
 *
 * @param filename		the config file to lex and dump
 * @result				the config's lexer dump as a string, must be freed with g_string_free afterwards
 */
API GString *lexConfigFile(char *filename)
{
	Config *config = allocateObject(Config);

	config->name = filename;
	config->resource = fopen(filename, "r");
	config->read = &configFileRead;
	config->unread = &configFileUnread;
	config->prelude = 0;

	GString *ret = dumpLex(config);

	fclose(config->resource);
	free(config);

	return ret;
}

/**
 * Lexes a config and dumps the result
 *
 * @param config		the config to lex and dump
 * @result				the config's lexer dump as a string, must be freed with g_string_free afterwards
 */
static GString *dumpLex(Config *config)
{
	int lexx;
	YYSTYPE val;
	YYLTYPE loc;

	GString *ret = g_string_new("");
	g_string_append_printf(ret, "Lexer dump for %s:\n", config->name);

	memset(&val, 0, sizeof(YYSTYPE));

	while((lexx = yylex(&val, &loc, config)) != 0) {
		switch(lexx) {
			case STRING:
				g_string_append_printf(ret, "<string=\"%s\"> ", val.string);
			break;
			case INTEGER:
				g_string_append_printf(ret, "<integer=%d> ", val.integer);
			break;
			case FLOAT_NUMBER:
				g_string_append_printf(ret, "<float=%f> ", val.float_number);
			break;
			case '\n':
				g_string_append(ret, "'\\n' ");
			break;
			default:
				g_string_append_printf(ret, "'%c' ", lexx);
			break;
		}

		memset(&val, 0, sizeof(YYSTYPE));
	}

	return ret;
}
