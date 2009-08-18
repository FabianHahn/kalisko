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


#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "source.h"
#include "tree_convert.h"
#include "tree_dump.h"

static void printLetterDumper(void *, const char *, ...);
static void printWordDumper(void *, const char *, ...);
static void fileLetterDumper(void *, const char *, ...);
static void fileWordDumper(void *, const char *, ...);

API void dumpMarkovTreeLevel(GTree *tree, int level, DumpOutput *dump, void *data)
{
	MarkovStatsNode *current_node;

	if(tree == NULL) { // Reached a leaf
		return;
	}

	GArray *array = convertTreeToArray(tree, sizeof(MarkovStatsNode *)); // Convert to array

	for(int i = 0; i < array->len; i++) { // Loop over tree items
		current_node = g_array_index(array, MarkovStatsNode *, i);

		for(int j = 0; j < level; j++) {
			dump(data, "\t");
		}

		dump(data, NULL, current_node->symbol);
		dump(data, ": %d\n", current_node->count);

		dumpMarkovTreeLevel(current_node->substats, level + 1, dump, data);
	}

	g_array_free(array, TRUE); // Free the helper array
}

API void printMarkovLetterTree(GTree *tree)
{
	dumpMarkovTreeLevel(tree, 0, &printLetterDumper, NULL);
}

API void printMarkovWordTree(GTree *tree)
{
	dumpMarkovTreeLevel(tree, 0, &printWordDumper, NULL);
}

API int fileDumpMarkovLetterTree(GTree *tree, char *filename)
{
	FILE *file = fopen(filename, "w+");

	if(file == NULL) { // Could not open file
		fprintf(stderr, "Could not open file %s for writing!\n", filename);
		return FALSE;
	}

	dumpMarkovTreeLevel(tree, 0, &fileLetterDumper, file);

	fclose(file);

	return TRUE;
}

API int fileDumpMarkovWordTree(GTree *tree, char *filename)
{
	FILE *file = fopen(filename, "w+");

	if(file == NULL) { // Could not open file
		fprintf(stderr, "Could not open file %s for writing!\n", filename);
		return FALSE;
	}

	dumpMarkovTreeLevel(tree, 0, &fileWordDumper, file);

	fclose(file);

	return TRUE;
}

static void printLetterDumper(void *data, const char *format, ...)
{
	va_list args;
	char *cp;

	va_start(args, format);

	if(format != NULL) { // There is a format string
		// Call printf
		vprintf(format, args);
		va_end(args);
	} else { // There is no format string
		cp = va_arg(args, char *);
		printf("%c", *cp);
	}
}

static void printWordDumper(void *data, const char *format, ...)
{
	va_list args;
	char *string;

	va_start(args, format);

	if(format != NULL) { // There is a format string
		// Call printf
		vprintf(format, args);
		va_end(args);
	} else { // There is no format string
		string = va_arg(args, char *);
		printf("%s", string);
	}
}

static void fileLetterDumper(void *data, const char *format, ...)
{
	FILE *file = (FILE *) data;

	va_list args;
	char *cp;

	va_start(args, format);

	if(format != NULL) { // There is a format string
		// Call fprintf
		vfprintf(file, format, args);
		va_end(args);
	} else { // There is no format string
		cp = va_arg(args, char *);
		fprintf(file, "%c", *cp);
	}
}

static void fileWordDumper(void *data, const char *format, ...)
{
	FILE *file = (FILE *) data;

	va_list args;
	char *string;

	va_start(args, format);

	if(format != NULL) { // There is a format string
		// Call fprintf
		vfprintf(file, format, args);
		va_end(args);
	} else { // There is no format string
		string = va_arg(args, char *);
		fprintf(file, "%s", string);
	}
}
