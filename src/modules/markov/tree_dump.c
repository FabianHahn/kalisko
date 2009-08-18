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

static void PrintLetterDumper(void *, const char *, ...);
static void PrintWordDumper(void *, const char *, ...);
static void FileLetterDumper(void *, const char *, ...);
static void FileWordDumper(void *, const char *, ...);

API void DumpMarkovTreeLevel(GTree *tree, int level, DumpOutput *dump, void *data)
{
	MarkovStatsNode *current_node;

	if(tree == NULL) { // Reached a leaf
		return;
	}

	GArray *array = ConvertTreeToArray(tree, sizeof(MarkovStatsNode *)); // Convert to array

	for(int i = 0; i < array->len; i++) { // Loop over tree items
		current_node = g_array_index(array, MarkovStatsNode *, i);

		for(int j = 0; j < level; j++) {
			dump(data, "\t");
		}

		dump(data, NULL, current_node->symbol);
		dump(data, ": %d\n", current_node->count);

		DumpMarkovTreeLevel(current_node->substats, level + 1, dump, data);
	}

	g_array_free(array, TRUE); // Free the helper array
}

API void PrintMarkovLetterTree(GTree *tree)
{
	DumpMarkovTreeLevel(tree, 0, &PrintLetterDumper, NULL);
}


API void PrintMarkovWordTree(GTree *tree)
{
	DumpMarkovTreeLevel(tree, 0, &PrintWordDumper, NULL);
}

API int FileDumpMarkovLetterTree(GTree *tree, char *filename)
{
	FILE *file = fopen(filename, "w+");

	if(file == NULL) { // Could not open file
		fprintf(stderr, "Could not open file %s for writing!\n", filename);
		return FALSE;
	}

	DumpMarkovTreeLevel(tree, 0, &FileLetterDumper, file);

	fclose(file);

	return TRUE;
}

API int FileDumpMarkovWordTree(GTree *tree, char *filename)
{
	FILE *file = fopen(filename, "w+");

	if(file == NULL) { // Could not open file
		fprintf(stderr, "Could not open file %s for writing!\n", filename);
		return FALSE;
	}

	DumpMarkovTreeLevel(tree, 0, &FileWordDumper, file);

	fclose(file);

	return TRUE;
}

static void PrintLetterDumper(void *data, const char *format, ...)
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

static void PrintWordDumper(void *data, const char *format, ...)
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

static void FileLetterDumper(void *data, const char *format, ...)
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

static void FileWordDumper(void *data, const char *format, ...)
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
