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
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "source.h"
#include "tree_convert.h"

static void FreeMarkovTree(GTree *tree);

API MarkovSource *CreateMarkovSource(int level, int elementSize, GCompareFunc comparer)
{
	MarkovSource *source = g_malloc(sizeof(MarkovSource));

	source->level = level;
	source->comparer = comparer;
	source->stats = g_tree_new(comparer);
	source->elementSize = elementSize;
	source->count = 0;

	return source;
}

API MarkovStatsNode *CreateMarkovStatsNode(MarkovSource *parent_source, void *symbol)
{
	MarkovStatsNode *node = g_malloc(sizeof(MarkovStatsNode));

	node->symbol = symbol;
	node->count = 0;
	node->substats = NULL;
	node->source = parent_source;

	return node;
}

API void ReadMarkovSymbol(MarkovSource *source, GQueue *symbol_queue)
{
	GTree *current_tree = source->stats;
	MarkovStatsNode *current_node;
	void *current_symbol;

	// Abort if symbol list too short
	if(symbol_queue->length != source->level + 1) {
		return;
	}

	for(int i = 0; i < symbol_queue->length; i++) { // For each symbol
		current_symbol = g_queue_peek_nth(symbol_queue, i); // Fetch symbol

		current_node = (MarkovStatsNode *) g_tree_lookup(current_tree, current_symbol); // Locate symbol in current tree

		if(current_node == NULL) { // Node doesn't exist yet
			current_node = CreateMarkovStatsNode(source, current_symbol); // Create it

			g_tree_insert(current_tree, current_symbol, current_node); // Insert it
		}

		// Increase node count
		current_node->count++;

		if(i < symbol_queue->length - 1) { // List goes on, switch to next subtree
			if(current_node->substats == NULL) { // Subtree doesn't exist yet
				current_node->substats = g_tree_new(source->comparer); // Create it
			}

			current_tree = current_node->substats; // Set new current tree
		}
	}

	// Increase source node count
	source->count++;
}

API void FreeMarkovSource(MarkovSource *source)
{
	FreeMarkovTree(source->stats);
	g_free(source); // Free the source itself
}

static void FreeMarkovTree(GTree *tree)
{
	MarkovStatsNode *current_node;

	if(tree == NULL) { // Reached a leaf
		return;
	}

	GArray *array = ConvertTreeToArray(tree, sizeof(MarkovStatsNode *)); // Convert to array

	for(int i = 0; i < array->len; i++) { // Loop over tree items
		current_node = g_array_index(array, MarkovStatsNode *, i);
		FreeMarkovTree(current_node->substats);
		g_free(current_node); // Free the symbol
	}

	g_array_free(array, TRUE); // Free the helper array
	g_tree_destroy(tree); // Destroy the tree itself
}
