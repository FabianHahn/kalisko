/**
 * @file config_lexer.c
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

#include "api.h"
#include "config.h"
#include "config_parser.h"
#include "config_lexer.h"

API int yylex(YYSTYPE *lval/*, YYLTYPE *lloc*/, ConfigFile *config)
{
	int c;
	bool escaping = false;
	bool reading_string = false;
	bool string_is_delimited = false;
	bool reading_numeric = false;
	bool numeric_is_float = false;
	char assemble[CONFIG_MAX_STRING_LENGTH];
	int i = 0;

	while(true) {
		c = config->read(config);

		switch(c) {
			case EOF:
			case '\0':
				return 0;
			break;
			case '[':
			case ']':
			case '(':
			case ')':
			case '{':
			case '}':
			case '=':
			case '\n':
				if(reading_string) {
					if(!string_is_delimited) { // end of undelimited string reached
						assemble[i] = '\0';
						lval->string = strdup(assemble);
						config->unread(config, c); // push back to stream
						return STRING;
					} // else just continue reading
				} else if(reading_numeric) {
					if(numeric_is_float) { // end of float reached
						assemble[i] = '\0';
						lval->float_number = atof(assemble);
						config->unread(config, c); // push back to stream
						return FLOAT_NUMBER;
					} else { // end of int reached
						assemble[i] = '\0';
						lval->integer = atoi(assemble);
						config->unread(config, c); // push back to stream
						return INTEGER;
					}
				} else {
					return c;
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
					return 0; // error
				} else if(isspace(c)) { // end of non delimited string reached
					assemble[i] = '\0';
					lval->string = strdup(assemble);
					return STRING;
				}
			}
		} else if(reading_numeric) {
			if(c == '.') { // delimiter
				if(numeric_is_float) {
					return 0; // error
				} else {
					numeric_is_float = true;
				}
			} else if(isspace(c)) { // whitespace, end of number
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
				continue;
			}
		} else { // We're starting to read now!
			if(isspace(c)) { // whitespace
				continue; // just ignore it
			} else if(isdigit(c)) { // reading an int or a float number
				reading_numeric = true;
			} else { // reading a string
				reading_string = true;

				if(c == '"') { // starting a delimited string
					string_is_delimited = true;
					continue;
				} else if(c == '\\') { // escape character not allowed in non-delimited string
					return 0; // error
				} // else just continue reading the non-delimited string
			}
		}

		if(escaping) { // escape character wasn't used
			return 0; // error
		}

		assemble[i++] = c;

		if(i >= CONFIG_MAX_STRING_LENGTH) {
			return 0; // error
		}
	}
}
