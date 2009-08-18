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
#include <string.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "source.h"
#include "file_word_source.h"
#include "tree_convert.h"

typedef struct {
	char c;
	int level;
} ReadLevelChar;

static ReadLevelChar ReadDumpFileWordLevel(MarkovFileWordSource *, FILE *, GTree *, int);

API MarkovFileWordSource *CreateMarkovFileWordSource(char *filename, int level)
{
	int read = 0;
	char c;
	char *symbol;
	GString *word = g_string_new("");
	int circled = -1;

	MarkovFileWordSource *fls = g_malloc(sizeof(MarkovFileWordSource));

	MarkovSource *source = CreateMarkovSource(level, sizeof(char *), &CompareWords);
	GTree *symbols = g_tree_new(&CompareWords);
	GQueue *symbol_queue = g_queue_new();
	GQueue *circular_queue = g_queue_new(); // Create circular queue whith is used to begin reading from start again level times after EOF

	fls->source = source;
	fls->symbols = symbols;

	FILE *file = fopen(filename, "r");

	if(file == NULL) { // Could not open file
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

		if(read == EOF) { // continue circling
			circled++;
			symbol = (char *) g_queue_pop_tail(circular_queue); // Pop circular queue element
			g_string_append(word, symbol); // Append symbol to current word
			c = ' '; // Fake word ending
		} else {
			c = read;
		}

		switch(c) {
			case ' ':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v': // Whitespace
				if(word->len) { // Word not empty
					symbol = g_tree_lookup(symbols, word->str);

					if(symbol == NULL) { // Symbol not yet in table
						symbol = g_malloc(sizeof(char) * (word->len + 1)); // Allocate space for it
						strcpy(symbol, word->str); // Copy word
						g_tree_insert(symbols, symbol, symbol); // Store it in symbol tree
					}

					if(circled + circular_queue->length + 1 < level) { // Circular queue not full enough
						g_queue_push_head(circular_queue, symbol); // Push current symbol into circle
					}

					g_queue_push_tail(symbol_queue, symbol); // Push symbol to queue

					if(symbol_queue->length > level) { // Queue full enough
						ReadMarkovSymbol(source, symbol_queue); // Let the markov source read the symbol
						g_queue_pop_head(symbol_queue); // Pop the first symbol
					}

					// Clear string
					g_string_free(word, TRUE);
					word = g_string_new("");
				}
			break;
			case '(':
			case ')':
			case '[':
			case ']':
			case '<':
			case '>':
			case '.':
			case ';':
			case ':':
			case ',':
			case '"':
			case '\'':
			case '`':
			case '!':
			case '?': // Adjacent punctuation mark
				// Add word if existent
				if(word->len) { // Word not empty
					symbol = g_tree_lookup(symbols, word->str);

					if(symbol == NULL) { // Symbol not yet in table
						symbol = g_malloc(sizeof(char) * (word->len + 1)); // Allocate space for it
						strcpy(symbol, word->str); // Copy word
						g_tree_insert(symbols, symbol, symbol); // Store it in symbol tree
					}

					if(circled + circular_queue->length < level) { // Circular queue not full enough
						g_queue_push_head(circular_queue, symbol); // Push current symbol into circle
					}

					g_queue_push_tail(symbol_queue, symbol); // Push symbol to queue

					if(symbol_queue->length > level) { // Queue full enough
						ReadMarkovSymbol(source, symbol_queue); // Let the markov source read the symbol
						g_queue_pop_head(symbol_queue); // Pop the first symbol
					}

					// Clear string
					g_string_free(word, TRUE);
					word = g_string_new("");
				}

				// Add the char
				g_string_append_c(word, c);

				symbol = g_tree_lookup(symbols, word->str);

				if(symbol == NULL) { // Symbol not yet in table
					symbol = g_malloc(sizeof(char) * (word->len + 1)); // Allocate space for it
					strcpy(symbol, word->str); // Copy word
					g_tree_insert(symbols, symbol, symbol); // Store it in symbol tree
				}

				g_queue_push_tail(symbol_queue, symbol);

				if(symbol_queue->length > level) { // Queue full enough
					ReadMarkovSymbol(source, symbol_queue); // Let the markov source read the symbol
					g_queue_pop_head(symbol_queue); // Pop the first symbol
				}

				// Clear string
				g_string_free(word, TRUE);
				word = g_string_new("");
			break;
			default: // A char inside a word
				g_string_append_c(word, c);
			break;
		}
	}

	g_queue_free(symbol_queue); // Free the symbol queue
	g_queue_free(circular_queue); // Free the circular queue

	g_string_free(word, TRUE); // Free the word string

	fclose(file);

	return fls;
}

API MarkovFileWordSource *CreateMarkovDumpFileWordSource(FILE *file, int level)
{
	MarkovFileWordSource *fws = g_malloc(sizeof(MarkovFileWordSource));

	MarkovSource *source = CreateMarkovSource(level, sizeof(char *), &CompareWords);
	GTree *symbols = g_tree_new(&CompareWords);

	fws->source = source;
	fws->symbols = symbols;

	ReadLevelChar ret = ReadDumpFileWordLevel(fws, file, fws->source->stats, 0);

	if(ret.level == -1) { // Parse error
		return NULL;
	}

	return fws;
}

API void FreeMarkovFileWordSource(MarkovFileWordSource *fls)
{
	GArray *array = ConvertTreeToArray(fls->symbols, sizeof(char *)); // Convert symbol tree to array

	for(int i = 0; i < array->len; i++) { // Loop over tree items
		g_free(g_array_index(array, char *, i)); // Free the symbol
	}

	g_tree_destroy(fls->symbols); // Free symbol tree
	g_array_free(array, TRUE); // Free the array and its elements

	FreeMarkovSource(fls->source); // Free markov source
	g_free(fls); // Free markov source itself
}

API int CompareWords(const void *a, const void *b)
{
	char *ap = (char *) a;
	char *bp = (char *) b;

	return strcmp(ap, bp);
}


static ReadLevelChar ReadDumpFileWordLevel(MarkovFileWordSource *fws, FILE *file, GTree *current_tree, int current_level)
{
	int read = 0;
	int state = 0;
	char c;
	char *symbol;
	int in_symbol = FALSE;
	char last_symbolchar = 0;
	GString *snumber = NULL;
	GString *word = NULL;
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

		if(!in_symbol) { // No current symbol yet
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
					in_symbol = TRUE;
					word = g_string_new("");
					last_symbolchar = c;
					state = 0;
				}
			}
		} else {
			if(state == 0) { // Colon not read yet
				if(last_symbolchar == ':' && c == ' ') { // Symbol ended
					state++;
					snumber = g_string_new("");
				} else { // Symbol hasn't ended yet
					assert(last_symbolchar);
					g_string_append_c(word, last_symbolchar); // Append the last char
					last_symbolchar = c;
				}
			} else if(c == '\n') { // Number ended
				number = atoi(snumber->str); // Parse number
				g_string_free(snumber, TRUE); // Free helper string


				symbol = g_tree_lookup(fws->symbols, word->str); // Lookup symbol in symbols tree

				if(symbol == NULL) { // Symbol not yet in table
					symbol = g_malloc(sizeof(char) * (word->len + 1)); // Allocate space for it
					strcpy(symbol, word->str); // Copy word
					g_tree_insert(fws->symbols, symbol, symbol); // Store it in symbol tree
				}

				g_string_free(word, TRUE);

				// Create the node
				node = CreateMarkovStatsNode(fws->source, symbol);
				node->count = number;

				g_tree_insert(current_tree, symbol, node); // Insert node into current tree

				if(current_level < fws->source->level) { // Final markov level not reached yet
					node->substats = g_tree_new(fws->source->comparer); // Create subtree

					ret = ReadDumpFileWordLevel(fws, file, node->substats, current_level + 1);

					if(ret.c) { // Valid char
						state = ret.level;
						in_symbol = FALSE;
					} else { // We're done
						return ret; // Exit recursion
					}
				} else { // Final markov level reached
					state = 0;
					in_symbol = FALSE;
					fws->source->count += number; // Add leaf count to source count
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
