/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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

#include <glib.h>
#include "dll.h"

#include "http_parser.h"

#define MATCH_HTTP_REQUEST_LINE "^(GET|POST)[ ]+(.+)[ ]+HTTP/\\d\\.\\d$"
#define HTTP_GET "GET"
#define HTTP_POST "POST"

static int countParts(char **parts)
{
  int count = 0;
	for(char **iter = parts; *iter != NULL; ++iter) {
		++count;
	}
  return count;
}

// Parses a single parameter from a string of the form "key=value". Returns whether or not parsing was successful.
static void parseParameter(HttpRequest *request, char *keyvalue)
{
	GHashTable *params = request->parameters;
	char **parts = g_strsplit(keyvalue, "=", 0);
	if(countParts(parts) == 2) {
		char *unescaped_key = g_uri_unescape_string(parts[0], NULL);
		char *unescaped_value = g_uri_unescape_string(parts[1], NULL);

		if(unescaped_key == NULL || unescaped_value == NULL) {
			LOG_DEBUG("Failed to unescape %s skipping", keyvalue);
		} else {
			// TODO: handle the case where the key already has a value (using g_hash_table_replace)
			g_hash_table_replace(params, unescaped_key, unescaped_value);
		}
	} else {
    LOG_DEBUG("Not exactly one = in %s, skipping", keyvalue);
  }

	g_strfreev(parts);
}

// Parses parameters from a string of the form "key1=value1&key2=value2". Return whether or not parsing was successful.
static void parseParameters(HttpRequest *request, char *query_part)
{
	char **parts = g_strsplit(query_part, "&", 0);
	for(char **iter = parts; *iter != NULL; ++iter) {
		parseParameter(request, *iter);
	}
	g_strfreev(parts);
}

static void parseUri(HttpRequest *request, char *uri)
{
	LOG_DEBUG("Request URI is %s", uri);
	request->uri = g_strdup(uri);

	if(strstr(uri, "?") == NULL) {
		// Copy the entire uri as hierarchical part
	  request->hierarchical = g_uri_unescape_string(uri, NULL);
		if(request->hierarchical == NULL) {
			LOG_DEBUG("Failed to unescape hierarchical part %s", uri);
		}
	} else {
		// Extract parameters
		char **parts = g_strsplit(uri, "?", 0);
		if(countParts(parts) == 2) {
			// Unescape the hierarchical part (which is the part before the ?)
			// TODO: possibly disallow special characters not allowed as file names (replace second param)
			request->hierarchical = g_uri_unescape_string(parts[0], NULL);
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
  LOG_DEBUG("Request method is %s", method);
	if(strcmp(HTTP_GET, method) == 0) {
		request->method = HTTP_REQUEST_METHOD_GET;
	} else if(strcmp(HTTP_POST, method) == 0) {
		request->method = HTTP_REQUEST_METHOD_POST;
	}
}

/** Parses one line as an HTTP request. Can handle empty lines. */
API void parseLine(HttpRequest *request, char *line)
{
	// LOG_DEBUG("Parsing HTTP line: %s", line);
  // An empty line indicates the end of the request
	if(strlen(line) == 0) {
		request->parsing_complete = true;
		return;
	}

	// Only detect lines of the form <METHOD> <URI> HTTP/<NUMBER>
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

