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

#include "api.h"
#include "modules/string_util/string_util.h"

MODULE_NAME("string_util");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Util function for working with strings.");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

#define NEWLINE_CHARS "\r\n"

MODULE_INIT
{
	return true;
}

/*MODULE_FINALIZE
{
}*/

/**
 * Replaces all multiple spaces by a single one within the given string.
 *
 * @param str	The string in which the multiple whitespaces should be replaced.
 */
API void stripDuplicateWhitespace(char *str)
{
	size_t len = strlen(str) + 1 /* Consider nullbyte */;
	size_t whitespace; // To keep track of whitespaces
	size_t textlength; // Length of text without spaces to skip

	while(*str != '\0') {
		// Is there whitespace to eat?
		whitespace = strspn(str, WHITESPACE_CHARS);
		if(whitespace > 0) {
			// Keep one space (' ')
			str[0] = ' ';
			str++, whitespace--, len--;
			// Move everything that follows to the front
			memmove(str, str + whitespace, len -= whitespace);
		}

		// Now let's move forward to the next whitespace
		textlength = strcspn(str, WHITESPACE_CHARS);
		str += textlength, len -= textlength;
	}
}

/**
 * Replaces all multiple newlines by a single newline within the given string.
 *
 * @param str	The string in which the multiple whitespaces should be replaced.
 */
API void stripDuplicateNewlines(char *str)
{
	size_t len = strlen(str) + 1 /* Consider nullbyte */;
	size_t whitespace; // To keep track of whitespaces
	size_t textlength; // Length of text without spaces to skip

	while(*str != '\0') {
		// Is there whitespace to eat?
		whitespace = strspn(str, NEWLINE_CHARS);
		if(whitespace > 0) {
			// Keep one newline ('\n')
			str[0] = '\n';
			str++, whitespace--, len--;
			// Move everything that follows to the front
			memmove(str, str + whitespace, len -= whitespace);
		}

		// Now let's move forward to the next whitespace
		textlength = strcspn(str, NEWLINE_CHARS);
		str += textlength, len -= textlength;
	}
}
