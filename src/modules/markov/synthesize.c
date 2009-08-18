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
#include "synthesize.h"
#include "source.h"
#include "probability.h"
#include "tree_convert.h"

API MarkovSynthesizer *createMarkovSynthesizer(MarkovSource *source)
{
	GTree *current_tree = source->stats;
	GArray *current_array;
	MarkovStatsNode *node;
	int current_count = source->count;

	MarkovSynthesizer *synth = (MarkovSynthesizer *) g_malloc(sizeof(MarkovSynthesizer));

	synth->source = source;
	synth->queue = g_queue_new();

	initRandomizer();

	// Init queue
	for(int i = 0; i < source->level; i++) {
		current_array = convertTreeToArray(current_tree, sizeof(MarkovStatsNode *)); // Convert to array
		node = rollMarkovSymbol(current_count, current_array); // Roll a symbol

		if(node == NULL) { // Failed to roll a symbol
			return NULL;
		}

		// Push the symbol
		g_queue_push_tail(synth->queue, node->symbol);

		// Move on to next subtree
		current_tree = node->substats;
		current_count = node->count;

		g_array_free(current_array, TRUE); // Free the helper array
	}

	return synth;
}

API void freeMarkovSynthesizer(MarkovSynthesizer *synth)
{
	g_queue_free(synth->queue); // Free the queue
	g_free(synth); // Free the synthesizer itself
}

API void *synthesizeSymbol(MarkovSynthesizer *synth)
{
	GTree *current_tree =  synth->source->stats;
	GArray *array;
	MarkovStatsNode *current_node = NULL;
	int count = synth->source->count;
	void *queued;

	// Move down in the tree according to the current queue of the synthesizer
	for(int i = 0; i < synth->queue->length; i++) {
		queued = g_queue_peek_nth(synth->queue, i); // Get the current queue element

		if(queued == NULL) { // Could not read queue element
			fprintf(stderr, "Synthesize error: Failed to read queue element\n");
			return NULL;
		}

		current_node = (MarkovStatsNode *) g_tree_lookup(current_tree, queued); // Fetch the node for the symbol

		if(current_node == NULL) { // Still couldn't fetch node for symbol
			fprintf(stderr, "Synthesize error: Failed to fetch node for symbol\n");
			return NULL;
		}

		// Move to next tree
		count = current_node->count;
		current_tree = current_node->substats;
	}

	array = convertTreeToArray(current_tree, sizeof(MarkovStatsNode *)); // Convert to array
	current_node = rollMarkovSymbol(count, array); // Roll a symbol

	if(current_node == NULL) { // We've haven't got a symbol :(
		fprintf(stderr, "Synthesize error: Rolling for symbol failed\n");
		return NULL;
	}

	g_queue_push_tail(synth->queue, current_node->symbol); // Add new symbol to queue
	g_queue_pop_head(synth->queue); // Pop first symbol

	g_array_free(array, TRUE); // Free the helper array

	return current_node->symbol; // Return synthesized symbol
}
