/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *findSecondLast(const char *string, const char *delimiters)
{
	int i;
	char **parts = g_strsplit_set(string, delimiters, 0); // Split by delimiters

	for(i = 0; parts[i] != NULL; i++); // Find the NULL terminator

	char *result = NULL;

	if(i >= 2) {
		result = strdup(parts[i-2]);
	}

	g_strfreev(parts);
	return result;
}

static char *getModuleName(const char *filename)
{
	return findSecondLast(filename, "/\\");
}

static char *getHeaderName(const char *filename)
{
	return findSecondLast(filename, ".");
}

int main(int argc, char **argv)
{
	GMemVTable *table = malloc(sizeof(GMemVTable));
	table->malloc = &malloc;
	table->realloc = &realloc;
	table->free = &free;
	table->calloc = NULL;
	table->try_malloc = NULL;
	table->try_realloc = NULL;
	g_mem_set_vtable(table);

	if(argc != 2) {
		fprintf(stderr, "The Kalisko Interface Compiler (KIC)\n");
		fprintf(stderr, "Usage: kic FILE\n");
		return EXIT_FAILURE;
	}

	const char *filename = argv[1];

	if(!g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
		fprintf(stderr, "Error: Could not find specified file '%s'\n", filename);
		return EXIT_FAILURE;
	}

	char *file;
	gsize size;

	if(!g_file_get_contents(filename, &file, &size, NULL)) {
		fprintf(stderr, "Error: Failed to open specified file '%s'\n", filename);
		return EXIT_FAILURE;
	}

	char **lines = g_strsplit(file, "\n", 0);
	free(file);
	unsigned int lineCount = 0;
	for(char **iter = lines; *iter != NULL; iter++) {
		lineCount++;
	}

	GError *error = NULL;
	GRegex *functionRegex = g_regex_new("^API ([^(]+[ *])(\\S+)\\(([^)]*)\\)(.*)", 0, 0, &error);

	if(error != NULL) {
		fprintf(stderr, "Error: Failed to create API regex");
		g_strfreev(lines);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	char *moduleName = getModuleName(filename);

	if(moduleName == NULL) { // core
		moduleName = strdup("");
	}

	GString *result = g_string_new("");
	g_string_append_printf(result, "// Compiled interface header for module '%s' generated by the Kalisko Interface Compiler (KIC)\n", moduleName);
	GTimeVal now;
	g_get_current_time(&now);
	char *date = g_time_val_to_iso8601(&now);
	g_string_append_printf(result, "// Created at: %s\n", date);
	free(date);
	g_string_append(result, "// DO NOT EDIT THIS FILE, ALL CHANGES WILL BE DISCARDED\n");

	for(unsigned int i = 0; i < lineCount; i++) {
		GMatchInfo *matches = NULL;
		char *line = lines[i];

		if(g_regex_match(functionRegex, line, 0, &matches)) {
			char *returnType = g_strchomp(g_match_info_fetch(matches, 1));
			char *functionName = g_match_info_fetch(matches, 2);
			char *argumentTypes = g_match_info_fetch(matches, 3);
			char *trailing = g_match_info_fetch(matches, 4);

			g_string_append_printf(result, "#ifdef WIN32\n");
			g_string_append_printf(result, "\t#ifdef API\n");
			g_string_append_printf(result, "\t\t__declspec(dllexport) %s %s(%s)%s\n", returnType, functionName, argumentTypes, trailing);
			g_string_append_printf(result, "\t#else\n");
			g_string_append_printf(result, "\t\t#define %s ((%s (*)(%s)) GET_API_FUNCTION(%s, %s))\n", functionName, returnType, argumentTypes, moduleName, functionName);
			g_string_append_printf(result, "\t#endif\n");
			g_string_append_printf(result, "#else\n");
			g_string_append_printf(result, "\t%s %s(%s)%s\n", returnType, functionName, argumentTypes, trailing);
			g_string_append_printf(result, "#endif\n");

			free(returnType);
			free(functionName);
			free(argumentTypes);
			free(trailing);
		} else {
			g_string_append_printf(result, "%s\n", line);
		}

		g_match_info_free(matches);
	}

	g_regex_unref(functionRegex);
	g_strfreev(lines);

	char *headerName = getHeaderName(filename);
	GString *resultFile = g_string_new(headerName);
	g_string_append(resultFile, ".h");

	if(g_strcmp0(resultFile->str, filename) == 0) {
		fprintf(stderr, "Error: Input and output file names match, aborting...\n");
		g_string_free(resultFile, TRUE);
		g_string_free(result, TRUE);
		free(moduleName);
		free(headerName);
		return EXIT_FAILURE;
	}

	if(!g_file_set_contents(resultFile->str, result->str, result->len, NULL)) {
		fprintf(stderr, "Error: Failed to write to output file '%s'\n", resultFile->str);
		g_string_free(resultFile, TRUE);
		g_string_free(result, TRUE);
		free(moduleName);
		free(headerName);
		return EXIT_FAILURE;
	}

	g_string_free(resultFile, TRUE);
	g_string_free(result, TRUE);
	free(moduleName);
	free(headerName);

	return EXIT_SUCCESS;
}
