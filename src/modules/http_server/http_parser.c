/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * Copyright (c) 2012, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *		 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *		 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *			 in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h> // atoi
#include <glib.h>
#include "dll.h"

#include "http_parser.h"

#define MATCH_HTTP_REQUEST_LINE "^(GET|POST)[ ]+(.+)[ ]+HTTP/\\d\\.\\d$"
#define MATCH_CONTENT_LENGTH_LINE "^Content-Length: ([\\d]+).*$"

#define HTTP_GET "GET"
#define HTTP_POST "POST"

static int countParts(char **parts);
static void parseParameter(HttpRequest *request, char *keyvalue);
static void parseParameters(HttpRequest *request, char *query_part);
static void replaceString(char **old, char *new);
static void parseUri(HttpRequest *request, char *uri);
static void parseMethod(HttpRequest *request, char *method);

static void tryParseMethodLine(HttpRequest *request, char *line);
static void tryParseContentLength(HttpRequest *request, char *line);

/**
 * Parses one line of an HTTP request and modifies the request accordingly. Can handle empty lines.
 */
API void parseHttpRequestLine(HttpRequest *request, char *line)
{
	// LOG_DEBUG("Parsing HTTP line: %s", line);
	tryParseMethodLine(request, line);
	tryParseContentLength(request, line);

	// An empty line indicates the end of the request
	if(strlen(line) == 0) {
		request->got_empty_line = true;
	}
}

/**
 * Parses one line of an HTTP request and modifies the request accordingly. Can handle empty lines.
 */
API void parseHttpRequestBody(HttpRequest *request, char *body)
{
	parseParameters(request, body);
}


/**
 * Attempts to parse the given line as a method line, i.e. a line of the form <METHOD> <URI> HTTP/<NUMBER>.
 */
static void tryParseMethodLine(HttpRequest *request, char *line)
{
	GRegex *regexp = g_regex_new(MATCH_HTTP_REQUEST_LINE, 0, 0, NULL);
	GMatchInfo *match_info;

	if(g_regex_match(regexp, line, 0, &match_info)) {
		char *method = g_match_info_fetch(match_info, 1);
		parseMethod(request, method);
		free(method);

		char *uri = g_match_info_fetch(match_info, 2);
		parseUri(request, uri);
		free(uri);
	}

	g_match_info_free(match_info);
	g_regex_unref(regexp);
}

/**
 * Attempts to parse the given line as a Content-Length line, i.e. a line of the form Content-length: XX.
 */
static void tryParseContentLength(HttpRequest *request, char *line)
{
	GRegex *regexp = g_regex_new(MATCH_CONTENT_LENGTH_LINE, 0, 0, NULL);
	GMatchInfo *match_info;

	if(g_regex_match(regexp, line, 0, &match_info)) {
		char *content_length_str = g_match_info_fetch(match_info, 1);

		// Returns 0 on failure, which is fine for us.
		request->content_length = atoi(content_length_str);
		free(content_length_str);
	}

	g_match_info_free(match_info);
	g_regex_unref(regexp);
}

static int countParts(char **parts)
{
	int count = 0;
	for(char **iter = parts; *iter != NULL; ++iter) {
		++count;
	}
	return count;
}

/**
 * Parses a single parameter from a string of the form "key=value". Returns whether or not parsing was successful.
 */
static void parseParameter(HttpRequest *request, char *keyvalue)
{
	GHashTable *params = request->parameters;
	char **parts = g_strsplit(keyvalue, "=", 0);
	if(countParts(parts) != 2) {
		LOG_DEBUG("Not exactly one = in %s, skipping", keyvalue);
	} else {
		char *unescaped_key = g_uri_unescape_string(parts[0], NULL);
		char *unescaped_value = g_uri_unescape_string(parts[1], NULL);

		if(unescaped_key == NULL || unescaped_value == NULL) {
			LOG_DEBUG("Failed to unescape %s skipping", keyvalue);
			free(unescaped_key);
			free(unescaped_value);
		} else {
			g_hash_table_replace(params, unescaped_key, unescaped_value); // Frees an old key or value if present
		}
	}

	g_strfreev(parts);
}

/**
 * Parses parameters from a string of the form "key1=value1&key2=value2". Return whether or not parsing was successful.
 */
static void parseParameters(HttpRequest *request, char *query_part)
{
	char **parts = g_strsplit(query_part, "&", 0);
	for(char **iter = parts; *iter != NULL; ++iter) {
		parseParameter(request, *iter);
	}
	g_strfreev(parts);
}

/**
 * Convenience method to free the old pointer if necessary and have it point to the new string
 */
static void replaceString(char **old, char *new)
{
	free(*old);
	*old = new;
}

static void parseUri(HttpRequest *request, char *uri)
{
	LOG_DEBUG("Request URI is %s", uri);
	replaceString(&request->uri, g_strdup(uri));

	if(strstr(uri, "?") == NULL) {
		// Copy the entire uri as hierarchical part
		replaceString(&request->hierarchical, g_uri_unescape_string(uri, NULL));
		if(request->hierarchical == NULL) {
			LOG_DEBUG("Failed to unescape hierarchical part %s", uri);
		}
	} else {
		// Extract parameters
		char **parts = g_strsplit(uri, "?", 0);
		if(countParts(parts) == 2) {
			// Unescape the hierarchical part (which is the part before the ?)
			// TODO: possibly disallow special characters not allowed as file names (replace second param)
			replaceString(&request->hierarchical, g_uri_unescape_string(parts[0], NULL));
			if(request->hierarchical == NULL) {
				LOG_DEBUG("Failed to unescape hierarchical part %s", parts[0]);
			}

			// Parse the parameter part
			// TODO: handle a fragment part
			parseParameters(request, parts[1]);
		} else {
		}
		g_strfreev(parts);
	}
}

static void parseMethod(HttpRequest *request, char *method)
{
	if(strcmp(HTTP_GET, method) == 0) {
		request->method = HTTP_REQUEST_METHOD_GET;
		LOG_DEBUG("Request method is %s", HTTP_GET);
	} else if(strcmp(HTTP_POST, method) == 0) {
		request->method = HTTP_REQUEST_METHOD_POST;
		LOG_DEBUG("Request method is %s", HTTP_POST);
	} else {
		LOG_WARNING("Unrecognized request method %s", method);
	}
}
