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

#include <math.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "quadtree.h"

MODULE_NAME("quadtree");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a quad tree data structure");
MODULE_VERSION(0, 7, 19);
MODULE_BCVERSION(0, 7, 17);
MODULE_NODEPS;

static void *loadQuadtreeNodeDataRec(Quadtree *tree, QuadtreeNode *node, double time);
static void *lookupQuadtreeRec(Quadtree *tree, QuadtreeNode *node, double time, double x, double y, unsigned int level);
static QuadtreeNode *lookupQuadtreeNodeRec(Quadtree *tree, QuadtreeNode *node, double time, double x, double y, unsigned int level);
static void fillTreeNodes(Quadtree *tree, QuadtreeNode *node, double time);
static void pruneQuadtreeNode(Quadtree *tree, QuadtreeNode *node, double time, unsigned int *target);
static void dumpQuadtreeNode(Quadtree *tree, QuadtreeNode *node, GString *string, unsigned int level);
static void freeQuadtreeNode(Quadtree *tree, QuadtreeNode *node);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{
}

/**
 * Creates a new quadtree
 *
 * @param leafSize				the leaf size of the quadtree
 * @param capacity				the caching capacity of the quadtree
 * @param load					the load function to use for the quadtree data
 * @param free					the free function to use for the quadtree data
 * @param preloadChildData		specifies whether the load function expects its child nodes to be already loaded
 * @result						the created quadtree
 */
API Quadtree *createQuadtree(unsigned int leafSize, unsigned int capacity, QuadtreeDataLoadFunction *load, QuadtreeDataFreeFunction *free, bool preloadChildData)
{
	Quadtree *quadtree = ALLOCATE_OBJECT(Quadtree);
	quadtree->leafSize = leafSize;
	quadtree->capacity = capacity;
	quadtree->load = load;
	quadtree->free = free;
	quadtree->preloadChildData = preloadChildData;
	quadtree->pruneFactor = 0.75f;
	quadtree->root = ALLOCATE_OBJECT(QuadtreeNode);
	quadtree->root->x = 0;
	quadtree->root->y = 0;
	quadtree->root->weight = 0;
	quadtree->root->time = $$(double, getMicroTime)();
	quadtree->root->level = 0;
	quadtree->root->children[0] = NULL;
	quadtree->root->children[1] = NULL;
	quadtree->root->children[2] = NULL;
	quadtree->root->children[3] = NULL;
	quadtree->root->data = NULL;

	return quadtree;
}

/**
 * Expands a quadtree to cover a specific point by adding new tree nodes
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 */
API void expandQuadtree(Quadtree *tree, double x, double y)
{
	if(quadtreeContainsPoint2D(tree, x, y)) {
		return; // nothing to do
	}

	double time = $$(double, getMicroTime)();
	QuadtreeAABB2D box = quadtreeNodeAABB2D(tree, tree->root);
	bool isLowerX = x < box.minX;
	bool isLowerY = y < box.minY;

	LOG_DEBUG("Expanding quadtree from range [%d,%d]x[%d,%d] to cover point (%f,%f)", box.minX, box.maxX, box.minY, box.maxY, x, y);

	QuadtreeNode *newRoot = ALLOCATE_OBJECT(QuadtreeNode);
	newRoot->time = time;
	newRoot->level = tree->root->level + 1;
	newRoot->data = NULL;
	newRoot->children[0] = NULL;
	newRoot->children[1] = NULL;
	newRoot->children[2] = NULL;
	newRoot->children[3] = NULL;

	unsigned int scale = quadtreeNodeScale(tree->root);
	if(isLowerX && isLowerY) { // old node becomes top right node of new root
		newRoot->x = tree->root->x - scale;
		newRoot->y = tree->root->y - scale;
		newRoot->children[3] = tree->root;
	} else if(isLowerX && !isLowerY) { // old node becomes bottom right node of new root
		newRoot->x = tree->root->x - scale;
		newRoot->y = tree->root->y;
		newRoot->children[1] = tree->root;
	} else if(!isLowerX && isLowerY) { // old node becomes top left node of new root
		newRoot->x = tree->root->x;
		newRoot->y = tree->root->y - scale;
		newRoot->children[2] = tree->root;
	} else { // old node becomes bottom left node of new root
		newRoot->x = tree->root->x;
		newRoot->y = tree->root->y;
		newRoot->children[0] = tree->root;
	}

	tree->root = newRoot;
	fillTreeNodes(tree, tree->root, time);

	// update node weight
	newRoot->weight = newRoot->children[0]->weight + newRoot->children[1]->weight + newRoot->children[2]->weight + newRoot->children[3]->weight + (quadtreeNodeDataIsLoaded(newRoot) ? 1 : 0);

	// recursively expand to make sure we include the point
	expandQuadtree(tree, x, y);
}

/**
 * Expands a quadtree to cover a specific point in world coordinates by adding new tree nodes
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param z				the z coordinate to lookup
 */
API void expandQuadtreeWorld(Quadtree *tree, double x, double z)
{
	expandQuadtree(tree, x * tree->leafSize, z * tree->leafSize);
}

/**
 * Loads the data for a quadtree node. If preloadChildData is set, recursively loads the child nodes' data first.
 * Note that this operation only updates the access time and the node weight of this node's subtree where appropriate, but not the values of the parent chain.
 * This means that if you want the tree weights to be consistent, you must track back the path to the root yourself and recursively update these nodes' weights.
 *
 * @param tree			the quadtree in which to load a node's data
 * @param node			the quadtree node for which to load the data
 * @result				the loaded node data
 */
API void *loadQuadtreeNodeData(Quadtree *tree, QuadtreeNode *node)
{
	double time = $$(double, getMicroTime)();
	return loadQuadtreeNodeDataRec(tree, node, time);
}

/**
 * Lookup a node's data in the quadtree
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @param level			the depth level at which to lookup the node data
 * @result				the looked up quadtree node's data
 */
API void *lookupQuadtree(Quadtree *tree, double x, double y, unsigned int level)
{
	if(!quadtreeContainsPoint2D(tree, x, y)) {
		// tree doesn't contain this part yet, so expand first...
		expandQuadtree(tree, x, y);
	}

	// Prune the quadtree if necessary
	pruneQuadtree(tree);

	// Perform the lookup
	double time = $$(double, getMicroTime)();
	void *data = lookupQuadtreeRec(tree, tree->root, time, x, y, level);

	// Return the looked up data
	return data;
}

/**
 * Lookup a node's data in the quadtree in world coordinates
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param z				the z coordinate to lookup
 * @param level			the depth level at which to lookup the node data
 * @result				the looked up quadtree node's data
 */
API void *lookupQuadtreeWorld(Quadtree *tree, double x, double z, unsigned int level)
{
	return lookupQuadtree(tree, x * tree->leafSize, z * tree->leafSize, level);
}

/**
 * Lookup a node in the quadtree
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @param level			the depth level at which to lookup the node
 * @result				the looked up quadtree node
 */
API QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, double x, double y, unsigned int level)
{
	if(!quadtreeContainsPoint2D(tree, x, y)) {
		expandQuadtree(tree, x, y);
	}

	double time = $$(double, getMicroTime)();
	return lookupQuadtreeNodeRec(tree, tree->root, time, x, y, level);
}

/**
 * Lookup a node in the quadtree in world coordinates
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param z				the z coordinate to lookup
 * @param level			the depth level at which to lookup the node
 * @result				the looked up quadtree node
 */
API QuadtreeNode *lookupQuadtreeNodeWorld(Quadtree *tree, double x, double z, unsigned int level)
{
	return lookupQuadtreeNode(tree, x * tree->leafSize, z * tree->leafSize, level);
}

/**
 * Prune a quadtree by unloading cached leaf node data
 *
 * @param tree		the quadtree to prune
 */
API void pruneQuadtree(Quadtree *tree)
{
	if(tree->root->weight <= (tree->capacity - 1)) { // make sure to prune already at capacity - 1 elements since we could lookup one more in a subsequent lookup
		return; // nothing to do
	}

	unsigned int target = tree->root->weight - floor(tree->capacity * tree->pruneFactor);

	LOG_DEBUG("Pruning %u quadtree nodes", target);

	double time = $$(double, getMicroTime)();
	pruneQuadtreeNode(tree, tree->root, time, &target);

	assert(tree->root->weight <= tree->capacity);
}

/**
 * Dumps the contents of a quadtree into a string
 *
 * @param tree			the tree to dump
 * @result				the string representation of the quadtree, must be freed after use
 */
API char *dumpQuadtree(Quadtree *tree)
{
	GString *string = g_string_new("");
	g_string_append_printf(string, "Quadtree: capacity = %u, leafsize = %u\n", tree->capacity, tree->leafSize);

	// dump all the nodes
	dumpQuadtreeNode(tree, tree->root, string, 0);

	// Free the gstring, but not the result
	char *result = string->str;
	g_string_free(string, false);

	return result;
}

/**
 * Frees a quadtree including all it's nodes and their loaded data
 *
 * @param tree			the quadtree to free
 */
API void freeQuadtree(Quadtree *tree)
{
	// free all the nodes first
	freeQuadtreeNode(tree, tree->root);

	// if all the nodes are freed, free the tree itself as well
	free(tree);
}

/**
 * Recursively loads the data for a quadtree node.
 * Note that this operation only updates the access time and the node weight of this node's subtree where appropriate, but not the values of the parent chain.
 * This means that if you want the tree weights to be consistent, you must track back the path to the root yourself and recursively update these nodes' weights.
 *
 * @param tree			the quadtree in which to load a node's data
 * @param node			the quadtree node for which to load the data
 * @param time			the current timestamp to set for caching
 * @result				the loaded node data
 */
static void *loadQuadtreeNodeDataRec(Quadtree *tree, QuadtreeNode *node, double time)
{
	if(quadtreeNodeDataIsLoaded(node)) {
		return node->data; // nothing to do if we're already loaded
	}

	QuadtreeAABB2D box = quadtreeNodeAABB2D(tree, node);

	if(node->level > 0 && tree->preloadChildData) {
		LOG_DEBUG("Preloading quadtree children of node with range [%d,%d]x[%d,%d]...", box.minX, box.maxX, box.minY, box.maxY);

		// Make sure the children are already loaded before loading this one
		for(unsigned int i = 0; i < 4; i++) {
			if(!quadtreeNodeDataIsLoaded(node->children[i])) {
				loadQuadtreeNodeDataRec(tree, node->children[i], time);
			}
		}
	}

	LOG_DEBUG("Loading quadtree node data for range [%d,%d]x[%d,%d] at level %u", box.minX, box.maxX, box.minY, box.maxY, node->level);
	node->data = tree->load(tree, node);

	// update node weight
	if(quadtreeNodeIsLeaf(node)) {
		node->weight = quadtreeNodeDataIsLoaded(node) ? 1 : 0;
	} else {
		node->weight = node->children[0]->weight + node->children[1]->weight + node->children[2]->weight + node->children[3]->weight + (quadtreeNodeDataIsLoaded(node) ? 1 : 0);
	}

	return node->data;
}

/**
 * Recursively lookup the data of a node in a quadtree. Make sure that the node actually contains the point you're looking for before calling this function.
 *
 * @param tree			the quadtree to lookup
 * @param node			the current node we're traversing
 * @param time			the current timestamp to set for caching
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @param level			the depth level at which to lookup the node data
 * @result				the looked up quadtree node data
 */
static void *lookupQuadtreeRec(Quadtree *tree, QuadtreeNode *node, double time, double x, double y, unsigned int level)
{
	assert(quadtreeNodeContainsPoint2D(tree, node, x, y));

	node->time = time; // update access time

	if(node->level <= level) { // we hit the desired level
		if(!quadtreeNodeDataIsLoaded(node)) { // make sure the data is loaded
			loadQuadtreeNodeDataRec(tree, node, time);
		}
		return node->data;
	} else {
		assert(!quadtreeNodeIsLeaf(node));

		int index = quadtreeNodeGetContainingChildIndex(tree, node, x, y);
		QuadtreeNode *child = node->children[index];
		void *data = lookupQuadtreeRec(tree, child, time, x, y, level);

		// update node weight
		node->weight = node->children[0]->weight + node->children[1]->weight + node->children[2]->weight + node->children[3]->weight + (quadtreeNodeDataIsLoaded(node) ? 1 : 0);

		// return looked up data
		return data;
	}
}

/**
 * Recursively lookup a node in a quadtree. Make sure that the node actually contains the point you're looking for before calling this function.
 *
 * @param tree			the quadtree to lookup
 * @param node			the current node we're traversing
 * @param time			the current timestamp to set for caching
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @param level			the depth level at which to lookup the node
 * @result				the looked up quadtree node
 */
static QuadtreeNode *lookupQuadtreeNodeRec(Quadtree *tree, QuadtreeNode *node, double time, double x, double y, unsigned int level)
{
	assert(quadtreeNodeContainsPoint2D(tree, node, x, y));

	node->time = time; // update access time

	if(node->level <= level) { // we hit the desired level
		return node;
	} else {
		assert(!quadtreeNodeIsLeaf(node));

		int index = quadtreeNodeGetContainingChildIndex(tree, node, x, y);
		QuadtreeNode *child = node->children[index];
		return lookupQuadtreeNodeRec(tree, child, time, x, y, level);
	}
}

/**
 * Recursively fill the tree with nodes
 *
 * @param tree			the tree to fill with nodes
 * @param node			the current node we're traversing
 * @param time			the current timestamp used for caching
 */
static void fillTreeNodes(Quadtree *tree, QuadtreeNode *node, double time)
{
	if(quadtreeNodeIsLeaf(node)) { // exit condition, reached leaf
		return;
	}

	for(int i = 0; i < 4; i++) {
		if(node->children[i] == NULL) { // if child node doesn't exist yet, create it
			node->children[i] = ALLOCATE_OBJECT(QuadtreeNode);
			node->children[i]->level = node->level - 1;
			node->children[i]->weight = 0;
			node->children[i]->time = time;

			unsigned int scale = quadtreeNodeScale(node->children[i]);
			node->children[i]->x = node->x + (i % 2) * scale;
			node->children[i]->y = node->y + ((i & 2) >> 1) * scale;

			// set content to null
			node->children[i]->data = NULL;
			node->children[i]->children[0] = NULL;
			node->children[i]->children[1] = NULL;
			node->children[i]->children[2] = NULL;
			node->children[i]->children[3] = NULL;
		}

		fillTreeNodes(tree, node->children[i], time); // recursively fill the child nodes
	}
}

/**
 * Recursively prunes a quadtree node by unloading cached leaf node data
 *
 * @param tree			the tree to prune
 * @param node			the current node we're traversing
 * @param time			the current timestamp used for caching
 * @param target		a pointer to the number of nodes we still have to prune
 */
static void pruneQuadtreeNode(Quadtree *tree, QuadtreeNode *node, double time, unsigned int *target)
{
	if(!quadtreeNodeIsLeaf(node)) { // prune lower levels first
		while(*target > 0) {
			// determine child that wasn't accessed the longest
			int minChild = -1;
			double minTime = time;
			for(int i = 0; i < 4; i++) {
				if(node->children[i]->weight > 0 && (minChild == -1 || node->children[i]->time <= minTime)) {
					minChild = i;
					minTime = node->children[i]->time;
				}
			}

			if(minChild == -1) { // all of the children were pruned
				break;
			}

			// prune the child with the oldest access time
			pruneQuadtreeNode(tree, node->children[minChild], time, target);
		}
	}

	// prune ourselves
	if(quadtreeNodeDataIsLoaded(node) && *target > 0) {
		QuadtreeAABB2D box = quadtreeNodeAABB2D(tree, node);
		LOG_DEBUG("Pruning quadtree node covering range [%d,%d]x[%d,%d] with access time %f", box.minX, box.maxX, box.minY, box.maxY, node->time);
		tree->free(tree, node->data);
		node->data = NULL;
		*target = *target - 1;
	}

	// update node weight
	if(quadtreeNodeIsLeaf(node)) {
		node->weight = quadtreeNodeDataIsLoaded(node) ? 1 : 0;
	} else {
		node->weight = node->children[0]->weight + node->children[1]->weight + node->children[2]->weight + node->children[3]->weight + (quadtreeNodeDataIsLoaded(node) ? 1 : 0);
	}
}

/**
 * Recursively dumps a quadtree node to a string
 *
 * @param tree			the tree to which the node belongs
 * @param node			the node we're currently traversing
 * @param string		the string to which the contents should be dumped
 * @param level			the current indentation level
 */
static void dumpQuadtreeNode(Quadtree *tree, QuadtreeNode *node, GString *string, unsigned int level)
{
	for(unsigned int i = 0; i < level; i++) { // indentation
		g_string_append_c(string, '\t');
	}

	QuadtreeAABB2D box = quadtreeNodeAABB2D(tree, node);
	g_string_append_printf(string, "Quadtree node: range = [%d,%d]x[%d,%d], weight = %u, time = %f, loaded = %s\n", box.minX, box.maxX, box.minY, box.maxY, node->weight, node->time, quadtreeNodeDataIsLoaded(node) ? "yes" : "no");

	if(!quadtreeNodeIsLeaf(node)) {
		for(unsigned int i = 0; i < 4; i++) {
			dumpQuadtreeNode(tree, node->children[i], string, level + 1);
		}
	}
}

/**
 * Recursively frees a quadtree node and the leaf nodes' loaded data
 *
 * @param tree			the tree to which the node belongs
 * @param node			the current node we're traversing to free
 */
static void freeQuadtreeNode(Quadtree *tree, QuadtreeNode *node)
{
	if(quadtreeNodeIsLeaf(node)) {
		if(quadtreeNodeDataIsLoaded(node)) {
			tree->free(tree, node->data); // free this node's data
		}
	} else {
		for(int i = 0; i < 4; i++) {
			freeQuadtreeNode(tree, node->children[i]);
		}
	}

	// free the node if all the children are or the data is freed
	free(node);
}
