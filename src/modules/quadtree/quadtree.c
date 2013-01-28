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
#define API
#include "quadtree.h"

MODULE_NAME("quadtree");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a quad tree data structure");
MODULE_VERSION(0, 12, 2);
MODULE_BCVERSION(0, 12, 0);
MODULE_NODEPS;

static QuadtreeNode *lookupQuadtreeNodeRec(Quadtree *tree, QuadtreeNode *node, double x, double y, unsigned int level);
static void fillTreeNodes(Quadtree *tree, QuadtreeNode *node);
static void dumpQuadtreeNode(Quadtree *tree, QuadtreeNode *node, GString *string, unsigned int level);
static void freeQuadtreeNode(Quadtree *tree, QuadtreeNode *node);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{
}

API Quadtree *createQuadtree(QuadtreeDataCreateFunction *create, QuadtreeDataFreeFunction *free)
{
	Quadtree *quadtree = ALLOCATE_OBJECT(Quadtree);
	quadtree->create = create;
	quadtree->free = free;
	quadtree->root = NULL;

	return quadtree;
}

API void reshapeQuadtree(Quadtree *tree, int rootX, int rootY, int rootLevel)
{
	// free all the existing nodes first
	freeQuadtreeNode(tree, tree->root);

	QuadtreeNode *newRoot = ALLOCATE_OBJECT(QuadtreeNode);
	newRoot->x = rootX;
	newRoot->y = rootY;
	newRoot->level = rootLevel;
	newRoot->data = NULL;
	newRoot->parent = NULL;
	newRoot->children[0] = NULL;
	newRoot->children[1] = NULL;
	newRoot->children[2] = NULL;
	newRoot->children[3] = NULL;
	tree->root = newRoot;

	tree->create(tree, newRoot);

	QuadtreeAABB box = quadtreeNodeAABB(tree->root);
	logInfo("Reshaping quadtree to range [%d,%d]x[%d,%d]", box.minX, box.maxX, box.minY, box.maxY);
	fillTreeNodes(tree, tree->root);
}

API void expandQuadtree(Quadtree *tree, double x, double y)
{
	if(tree->root == NULL) {
		tree->root = ALLOCATE_OBJECT(QuadtreeNode);
		tree->root->x = 0;
		tree->root->y = 0;
		tree->root->level = 0;
		tree->root->parent = NULL;
		tree->root->children[0] = NULL;
		tree->root->children[1] = NULL;
		tree->root->children[2] = NULL;
		tree->root->children[3] = NULL;
		tree->root->data = NULL;
		tree->create(tree, tree->root);
	}

	if(quadtreeContainsPoint(tree, x, y)) {
		return; // nothing to do
	}

	QuadtreeAABB box = quadtreeNodeAABB(tree->root);
	bool isLowerX = x < box.minX;
	bool isLowerY = y < box.minY;

	logInfo("Expanding quadtree from range [%d,%d]x[%d,%d] to cover point (%f,%f)", box.minX, box.maxX, box.minY, box.maxY, x, y);

	QuadtreeNode *newRoot = ALLOCATE_OBJECT(QuadtreeNode);
	newRoot->level = tree->root->level + 1;
	newRoot->data = NULL;
	newRoot->parent = NULL;
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

	tree->root->parent = newRoot;
	tree->root = newRoot;

	tree->create(tree, newRoot);

	fillTreeNodes(tree, tree->root);

	// recursively expand to make sure we include the point
	expandQuadtree(tree, x, y);
}

API QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, double x, double y, unsigned int level)
{
	if(!quadtreeContainsPoint(tree, x, y)) {
		expandQuadtree(tree, x, y);
	}

	return lookupQuadtreeNodeRec(tree, tree->root, x, y, level);
}

API char *dumpQuadtree(Quadtree *tree)
{
	GString *string = g_string_new("");
	g_string_append_printf(string, "Quadtree:\n");

	// dump all the nodes
	dumpQuadtreeNode(tree, tree->root, string, 0);

	// Free the gstring, but not the result
	char *result = string->str;
	g_string_free(string, false);

	return result;
}

API void freeQuadtree(Quadtree *tree)
{
	// free all the nodes first
	freeQuadtreeNode(tree, tree->root);

	// if all the nodes are freed, free the tree itself as well
	free(tree);
}

/**
 * Recursively lookup a node in a quadtree. Make sure that the node actually contains the point you're looking for before calling this function.
 *
 * @param tree			the quadtree to lookup
 * @param node			the current node we're traversing
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @param level			the depth level at which to lookup the node
 * @result				the looked up quadtree node
 */
static QuadtreeNode *lookupQuadtreeNodeRec(Quadtree *tree, QuadtreeNode *node, double x, double y, unsigned int level)
{
	assert(quadtreeNodeContainsPoint(node, x, y));

	if(node->level <= level) { // we hit the desired level
		return node;
	} else {
		assert(!quadtreeNodeIsLeaf(node));

		int index = quadtreeNodeGetContainingChildIndex(node, x, y);
		QuadtreeNode *child = node->children[index];
		return lookupQuadtreeNodeRec(tree, child, x, y, level);
	}
}

/**
 * Recursively fill the tree with nodes
 *
 * @param tree			the tree to fill with nodes
 * @param node			the current node we're traversing
 */
static void fillTreeNodes(Quadtree *tree, QuadtreeNode *node)
{
	if(quadtreeNodeIsLeaf(node)) { // exit condition, reached leaf
		return;
	}

	for(int i = 0; i < 4; i++) {
		if(node->children[i] == NULL) { // if child node doesn't exist yet, create it
			node->children[i] = ALLOCATE_OBJECT(QuadtreeNode);
			node->children[i]->level = node->level - 1;
			node->children[i]->parent = node;

			unsigned int scale = quadtreeNodeScale(node->children[i]);
			node->children[i]->x = node->x + (i % 2) * scale;
			node->children[i]->y = node->y + ((i & 2) >> 1) * scale;

			// set content to null
			node->children[i]->data = NULL;
			node->children[i]->children[0] = NULL;
			node->children[i]->children[1] = NULL;
			node->children[i]->children[2] = NULL;
			node->children[i]->children[3] = NULL;

			tree->create(tree, node->children[i]);
		}

		fillTreeNodes(tree, node->children[i]); // recursively fill the child nodes
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

	QuadtreeAABB box = quadtreeNodeAABB(node);
	g_string_append_printf(string, "Quadtree node: range = [%d,%d]x[%d,%d]\n", box.minX, box.maxX, box.minY, box.maxY);

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
	if(node == NULL) {
		return;
	}

	if(!quadtreeNodeIsLeaf(node)) {
		for(int i = 0; i < 4; i++) {
			freeQuadtreeNode(tree, node->children[i]);
		}
	}

	// free this node's data
	tree->free(tree, node->data);

	// free the node if all the children are or the data is freed
	free(node);
}
