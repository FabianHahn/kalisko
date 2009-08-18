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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "source.h"
#include "file_letter_source.h"
#include "tree_convert.h"

typedef struct {
	char c;
	int level;
} ReadLevelChar;

static ReadLevelChar readDumpFileLetterLevel(MarkovFileLetterSource *, FILE *, GTree *, int);

API MarkovFileLetterSource *createMarkovFileLetterSource(char *filename, int level)
{
	int read = 0;
	char c;
	char *cp;
	int circled = -1;

	MarkovFileLetterSource *fls = g_malloc(sizeof(MarkovFileLetterSource));

	MarkovSource *source = createMarkovSource(level, sizeof(char *), &compareLetters);
	GTree *symbols = g_tree_new(&compareLetters);
	GQueue *symbol_queue = g_queue_new();
	GQueue *circular_queue = g_queue_new(); // Create circular queue whith is used to begin reading from start again level times after EOF

	fls->source = source;
	fls->symbols = symbols;

	FILE *file = fopen(filename, "r");

	if(file == NULL) { // Could not open file
		fprintf(stderr, "Failed to open text source file %s!", filename);
		return NULL;
	}

	while(circled < level) { // Loop filelength + level times
		if(read != EOF) {
			read = fgetc(file); // Read a char

			if(read == EOF) {
				// Start circling
				circled = 0;
				continue;
			}
		}

		if(read == EOF) { // Continue circling
			circled++;
			cp = (char *) g_queue_pop_tail(circular_queue); // Pop circular queue element
			c = *cp;
		} else {
			c = read;
		}

		cp = g_tree_lookup(symbols, &c);

		if(cp == NULL) { // Symbol not yet in table
			cp = g_malloc(sizeof(char)); // Allocate space for it
			*cp = c; // Copy char
			g_tree_insert(symbols, cp, cp); // Store it in symbol tree
		}

		if(circled + circular_queue->length + 1 < level) { // Circular queue not full enough
			g_queue_push_head(circular_queue, cp); // Push current symbol into circle
		}

		g_queue_push_tail(symbol_queue, cp); // Push symbol to queue

		if(symbol_queue->length > level) { // Queue full enough
			readMarkovSymbol(source, symbol_queue); // Let the markov source read the symbol
			g_queue_pop_head(symbol_queue); // Pop the first symbol
		}
	}

	g_queue_free(symbol_queue); // Free the symbol queue
	g_queue_free(circular_queue); // Free the circular queue

	fclose(file);

	return fls;
}

API MarkovFileLetterSource *createMarkovDumpFileLetterSource(FILE *file, int level)
{
	MarkovFileLetterSource *fls = g_malloc(sizeof(MarkovFileLetterSource));

	MarkovSource *source = createMarkovSource(level, sizeof(char *), &compareLetters);
	GTree *symbols = g_tree_new(&compareLetters);

	fls->source = source;
	fls->symbols = symbols;

	ReadLevelChar ret = readDumpFileLetterLevel(fls, file, fls->source->stats, 0);

	if(ret.level == -1) { // Parse error
		return NULL;
	}

	return fls;
}

API void freeMarkovFileLetterSource(MarkovFileLetterSource *fls)
{
	GArray *array = convertTreeToArray(fls->symbols, sizeof(char *)); // Convert symbol tree to array

	for(int i = 0; i < array->len; i++) { // Loop over tree items
		g_free(g_array_index(array, char *, i)); // Free the symbol
	}

	g_tree_destroy(fls->symbols); // Free symbol tree
	g_array_free(array, TRUE); // Free the array and its elements

	freeMarkovSource(fls->source); // Free markov source
	g_free(fls); // Free markov source itself
}

API int compareLetters(const void *a, const void *b) {
	char *ap = (char *) a;
	char *bp = (char *) b;

	if(*ap == *bp) {
		return 0;
	} else if (*ap > *bp) {
		return 1;
	} else {
		return -1;
	}
}

static ReadLevelChar readDumpFileLetterLevel(MarkovFileLetterSource *fls, FILE *file, GTree *current_tree, int current_level)
{
	int read = 0;
	int state = 0;
	char c;
	char *cp;
	char current_symbol = 0;
	GString *snumber = NULL;
	int number;
	MarkovStatsNode *node;
	ReadLevelChar ret = {0, 0};

	while(ret.c || (read = fgetc(file)) != EOF) {
		if(ret.c) { // There is a preread char
			c = ret.c;
			ret.c = 0;
			ret.level = 0;
		} else { // Read from file
			c = read;
		}

		if(!current_symbol) { // No current symbol yet
			if(c == '\t') { // Next level
				state++;
			} else {
				if(state < current_level) { // Current level not reached
					ret.c = c;
					ret.level = state;
					return ret; // Return to the handler for the previous level
				} else if(state > current_level) { // Invalid dump file
					fprintf(stderr, "Parse error: Level jump at char %ld\n", ftell(file));
					ret.c = 0;
					ret.level = -1;
					return ret;
				} else {
					current_symbol = c;
					state = 0;
				}
			}
		} else {
			if(state == 0) { // Colon not read yet
				if(c != ':') {
					fprintf(stderr, "Parse error: Unexpected \'%c\' at char %ld, expected \':\'\n", c, ftell(file));
					ret.c = 0;
					ret.level = -1;
					return ret;
				} else { // Colon found
					state++;
					snumber = g_string_new("");
				}
			} else if(c == '\n') { // Number ended
				number = atoi(snumber->str); // Parse number
				g_string_free(snumber, TRUE); // Free helper string

				cp = g_tree_lookup(fls->symbols, &current_symbol); // Lookup char in symbols tree

				if(cp == NULL) { // Symbol not yet in table
					cp = g_malloc(sizeof(char)); // Allocate space for it
					*cp = current_symbol; // Copy char
					g_tree_insert(fls->symbols, cp, cp); // Store it in symbol tree
				}

				// Create the node
				node = createMarkovStatsNode(fls->source, cp);
				node->count = number;

				g_tree_insert(current_tree, cp, node); // Insert node into current tree

				if(current_level < fls->source->level) { // Final markov level not reached yet
					node->substats = g_tree_new(fls->source->comparer); // Create subtree

					ret = readDumpFileLetterLevel(fls, file, node->substats, current_level + 1);

					if(ret.c) { // Valid char
						state = ret.level;
						current_symbol = 0;
					} else { // We're done
						return ret; // Exit recursion
					}
				} else { // Final markov level reached
					state = 0;
					current_symbol = 0;
					fls->source->count += number; // Add leaf count to source count
				}
			} else {
				g_string_append_c(snumber, c); // Append the char to the number
			}
		}
	}

	ret.c = 0;
	ret.level = 0;

	return ret;
}
