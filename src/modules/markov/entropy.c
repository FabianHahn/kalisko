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

#include <glib.h>
#include <math.h>
#include "dll.h"
#include "api.h"
#include "source.h"
#include "tree_convert.h"
#include "probability.h"
#include "entropy.h"

static double getMarkovTreeEntropy(GTree *, int);

API double getMarkovEntropy(MarkovSource *source)
{
	return getMarkovTreeEntropy(source->stats, source->count);
}

static double getMarkovTreeEntropy(GTree *tree, int count)
{
	double entropy = 0.0;
	double probability;
	GArray *array = convertTreeToArray(tree, sizeof(MarkovStatsNode *));
	MarkovStatsNode *current_node;

	for(int i = 0; i < array->len; i++) {
		current_node = g_array_index(array, MarkovStatsNode *, i); // Fetch current node
		probability = getMarkovNodeProbability(count, current_node);

		if(current_node->substats != NULL) { // It's no leaf yet
			entropy += probability * getMarkovTreeEntropy(current_node->substats, current_node->count); // Recursively add to entropy
		} else { // Reached a leaf
			entropy -= probability * log(probability) / log(2);
		}
	}

	g_array_free(array, TRUE); // Free the helper array

	return entropy;
}
