/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "dll.h"
#include "module.h"

#include "api.h"
#include "modules/string_format/string_format.h"

MODULE_NAME("string_format");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Implements a comfortable string formatter.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_NODEPS;

/**
 * delim_start and delim_end specify how the tokens shall be recognized, i.e. how
 * they start and end. start="{" and end="}" means tokens will have the form {name}
 */
static const char *delim_start = "{";
static const char *delim_end = "}";

/**
 * It suffices to compute the length of the delimiters once and for all.
 */
static size_t delim_start_len;
static size_t delim_end_len;

MODULE_INIT
{
	// make sure the delimiters aren't empty
	delim_start_len = strlen(delim_start);
	delim_end_len = strlen(delim_end);

	if(delim_start_len == 0 || delim_end_len == 0) {
		LOG_WARNING("Either of the start and end delimiters is empty. Please set both to a specific value.");
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
}

/**
 * Formats a string with a set of tokens by the specified replacement strings.
 *
 * The parameter list after 'format' consists of key-value pairs of strings. This list
 * is terminated by 'NULL'. The first one is always the key which is searched in
 * 'format' and will be replaced by the value (second parameter). If in 'format'
 * a key exists that was not given by the parameter list, it will be removed in
 * the returned string.
 *
 * @code
 * 	format_string("{foo} {bar} {PI}", "foo", "Hello", "bar", "World", NULL);
 * @endcode
 * would return
 * @code
 * 	"Hello World "
 * @endcode
 *
 * @param format	A string with tokens to be replaced.
 * @param ...		List of key-value pairs of parameters terminated by NULL.
 */
API char *format_string(char *format, ...)
{
	va_list vl;
	va_start(vl, format);
	char *key, *value;

	// read all key->value pairs into a hashtable
	GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

	do {
		key = va_arg(vl, char*);
		if(key == NULL) {
			break;
		}
		value = va_arg(vl, char*);
		if(value == NULL) {
			break;
		}
		g_hash_table_insert(table, strdup(key), strdup(value));
	} while(1); // break condition is managed inside the loop

	char *last_pos = format;
	char *parse_position = format;
	GString *gstr = g_string_new(NULL);

	// walk over string, replacing as needed
	while((parse_position = strstr(parse_position, delim_start)) != NULL) {
		// add text between the last and the current position
		g_string_append_len(gstr, last_pos, parse_position - last_pos);
		// dismiss the start delimiter
		parse_position += delim_start_len;
		// find the end of the token
		char *token_end = strstr(parse_position, delim_end);
		if(token_end == NULL) {
			// no end found = no more tokens
			// add the unmatched start delimiter back to the string, everything beyond will be added after the loop
			g_string_append(gstr, delim_start);
			break;
		}
		// separate found key
		char *found_key = strndup(parse_position, token_end - parse_position);
		// see if a key with this name has been specified
		char *replacement = g_hash_table_lookup(table, found_key);
		if(replacement != NULL) {
			// replacement found, add it to the string
			g_string_append(gstr, replacement);
		}
		else {
			// there is no replacement for this key, just remove the key from the original string
		}

		// add the length of the found string to the current position
		parse_position = token_end + delim_end_len;
		last_pos = parse_position;
	}
	// after last_pos, no more tokens were found, add everything afterwards
	g_string_append(gstr, last_pos);

	// store string for returning
	char *ret = gstr->str;
	g_string_free(gstr, false);
	return ret;
}
