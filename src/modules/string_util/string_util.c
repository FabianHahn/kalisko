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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "dll.h"
#include "module.h"

#define API
#include "modules/string_util/string_util.h"

MODULE_NAME("string_util");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Util function for working with strings.");
MODULE_VERSION(0, 1, 5);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

#define NEWLINE_CHARS "\r\n"

const char *utf8ConversionCodesets[] = {"ISO-8859-1", NULL};

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

API void stripDuplicateWhitespace(char *str)
{
	size_t len = strlen(str) + 1 /* Consider nullbyte */;
	size_t whitespace = 0; // To keep track of whitespaces
	size_t textlength = 0; // Length of text without spaces to skip

	while(*str != '\0') {
		// Is there whitespace to eat?
		whitespace = strspn(str, WHITESPACE_CHARS);
		if(whitespace > 0) {
			// Keep one space (' ')
			str[0] = ' ';
			str++;
			whitespace--;
			len--;
			// Move everything that follows to the front
			len -= whitespace;
			memmove(str, str + whitespace, len);
		}

		// Now let's move forward to the next whitespace
		textlength = strcspn(str, WHITESPACE_CHARS);
		str += textlength;
		len -= textlength;
	}
}

API void stripDuplicateNewlines(char *str)
{
	size_t len = strlen(str) + 1 /* Consider nullbyte */;
	size_t whitespace = 0; // To keep track of whitespaces
	size_t textlength = 0; // Length of text without spaces to skip

	while(*str != '\0') {
		// Is there whitespace to eat?
		whitespace = strspn(str, NEWLINE_CHARS);
		if(whitespace > 0) {
			// Keep one newline ('\n')
			str[0] = '\n';
			str++;
			whitespace--;
			len--;
			// Move everything that follows to the front
			len -= whitespace;
			memmove(str, str + whitespace, len);
		}

		// Now let's move forward to the next whitespace
		textlength = strcspn(str, NEWLINE_CHARS);
		str += textlength;
		len -= textlength;
	}
}

API void convertToFilename(char *str)
{
	unsigned int len = strlen(str);

	for(unsigned int i = 0; i < len; i++) {
		if(!isalnum(str[i]) && str[i] != '#') {
			str[i] = '_';
		}
	}
}

/**
 * Converts a NUL terminated string to UTF-8 if needed, which can be useful for displaying it in GUI widgets and the like that require UTF-8
 *
 * @param string			the string to convert to UTF-8
 * @result					the converted string or NULL on failure
 */
char *convertToUtf8(const char *string)
{
	if(g_utf8_validate(string, -1, NULL)) { // already correct UTF-8
		return strdup(string);
	} else {
		// Try to convert to the current locale
		for(const char **iter = utf8ConversionCodesets; *iter != NULL; ++iter) {
			char *converted = g_convert(string, -1, "UTF-8", *iter, NULL, NULL, NULL); // try to convert to this codeset
			if(converted != NULL) {
				return converted; // if it worked, return the conversion
			}
		}
	}

	return NULL; // failure
}
