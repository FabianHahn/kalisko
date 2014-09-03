/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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


#ifndef STRING_UTIL_STRING_UTIL_H
#define STRING_UTIL_STRING_UTIL_H

/**
 * A char array of different ASCII whitespaces.
 */
const char WHITESPACE_CHARS[] = "\r\t\n\v\f ";

/**
 * Replaces all multiple spaces by a single one within the given string.
 *
 * @param str	The string in which the multiple whitespaces should be replaced.
 */
API void stripDuplicateWhitespace(char *str);

/**
 * Replaces all multiple newlines by a single newline within the given string.
 *
 * @param str	The string in which the multiple whitespaces should be replaced.
 */
API void stripDuplicateNewlines(char *str);

/**
 * Converts a string into a valid filename by replacing all non-alphanumeric non-# characters by underscores.
 *
 * @param str	The string to convert to a filename.
 */
API void convertToFilename(char *str);

/**
 * Converts a NUL terminated string to UTF-8 if needed, which can be useful for displaying it in GUI widgets and the like that require UTF-8.
 *
 * @param str   The string to convert to UTF-8.
 * @result      The converted string or NULL on failure.
 */
API char *convertToUtf8(const char *str);

/**
 * Attempts to parse a string as a set of comma-separated values and adds them to an array.
 *
 * @param str   The str to parse
 * @param out   An array of pointers to add the new strings to. Note that the new strins must be freed.
 * @result      The number of strings added to out.
 */
API size_t parseCommaSeparated(char *str, GPtrArray *out);

/**
 * Indents a string by prepending '\t' to every line
 *
 * @param input				the input string to indent
 * @param indentation		the indentation string (for example "\t")
 * @param levels			the number of levels by which to indent
 * @result					the indented string, must be freed by the caller
 */
API char *indentString(const char *input, const char *indentation, int levels);

#endif
