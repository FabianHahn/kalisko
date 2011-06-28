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
#include "dll.h"
#include "api.h"
#include "quadtree.h"

MODULE_NAME("quadtree");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a quad tree data structure");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static void fillTreeNodes(Quadtree *tree, QuadtreeNode *node);
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
 * @param leafSize			the leaf size of the quadtree
 * @result					the created quadtree
 */
API Quadtree *createQuadtree(double leafSize)
{
	Quadtree *quadtree = ALLOCATE_OBJECT(Quadtree);
	quadtree->leafSize = leafSize;
	quadtree->root = ALLOCATE_OBJECT(QuadtreeNode);
	quadtree->root->x = 0;
	quadtree->root->y = 0;
	quadtree->root->level = 0;
	quadtree->root->content.data = NULL;

	return quadtree;
}

/**
 * Expands a quad tree to cover a specific point
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @result				the freshly created quadtree node containing the point
 */
API QuadtreeNode *expandQuadtree(Quadtree *tree, double x, double y)
{
	if(quadtreeContainsPoint(tree, x, y)) { // nothing to do
		return lookupQuadtree(tree, x, y);
	}

	double span = quadtreeNodeSpan(tree, tree->root);
	bool isLowerX = x < tree->root->x;
	bool isLowerY = y < tree->root->y;

	QuadtreeNode *newRoot = ALLOCATE_OBJECT(QuadtreeNode);
	newRoot->level = tree->root->level + 1;
	newRoot->content.children[0] = NULL;
	newRoot->content.children[1] = NULL;
	newRoot->content.children[2] = NULL;
	newRoot->content.children[3] = NULL;

	if(isLowerX && isLowerY) { // old node becomes top right node of new root
		newRoot->x = tree->root->x - span;
		newRoot->y = tree->root->y - span;
		newRoot->content.children[3] = tree->root;
	} else if(isLowerX && !isLowerY) { // old node becomes bottom right node of new root
		newRoot->x = tree->root->x - span;
		newRoot->y = tree->root->y;
		newRoot->content.children[1] = tree->root;
	} else if(!isLowerX && isLowerY) { // old node becomes top left node of new root
		newRoot->x = tree->root->x;
		newRoot->y = tree->root->y - span;
		newRoot->content.children[2] = tree->root;
	} else { // old node becomes bottom left node of new root
		newRoot->x = tree->root->x;
		newRoot->y = tree->root->y;
		newRoot->content.children[0] = tree->root;
	}

	tree->root = newRoot;
	fillTreeNodes(tree, tree->root);

	return expandQuadtree(tree, x, y); // recursively expand to make sure we include the point
}

/**
 * Lookup a leaf node's data in the quadtree
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @result				the looked up quadtree node
 */
API void *lookupQuadtree(Quadtree *tree, double x, double y)
{
	QuadtreeNode *leaf;
	if(quadtreeContainsPoint(tree, x, y)) {
		leaf = lookupQuadtreeNode(tree, tree->root, x, y);
	} else {
		leaf = expandQuadtree(tree, x, y);
	}

	assert(quadtreeNodeIsLeaf(leaf));
	if(!quadtreeNodeDataIsLoaded(leaf)) {
		double span = quadtreeNodeSpan(tree, leaf);
		leaf->content.data = tree->load(x, y, span);
	}

	return leaf->content.data;
}

/**
 * Recursively lookup a leaf node in a quadtree
 *
 * @param tree			the quadtree to lookup
 * @param node			the current node we're traversing
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @result				the looked up quadtree node
 */
API QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, QuadtreeNode *node, double x, double y)
{
	assert(quadtreeNodeContainsPoint(tree, node, x, y));

	if(quadtreeNodeIsLeaf(node)) {
		return node;
	} else {
		int index = quadtreeNodeGetContainingChildIndex(tree, node, x, y);
		QuadtreeNode *child = node->content.children[index];
		return lookupQuadtreeNode(tree, child, x, y);
	}
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

	double span = quadtreeNodeSpan(tree, node);

	for(int i = 0; i < 4; i++) {
		if(node->content.children[i] == NULL) { // if child node doesn't exist yet, create it
			node->content.children[i] = ALLOCATE_OBJECT(QuadtreeNode);
			node->content.children[i]->level = node->level - 1;
			node->content.children[i]->x = node->x + (i % 2) * 0.5 * span;
			node->content.children[i]->y = node->y + (i & 2) * 0.5 * span;

			// set content to null
			if(quadtreeNodeIsLeaf(node->content.children[i])) {
				node->content.children[i]->content.data = NULL;
			} else {
				node->content.children[i]->content.children[0] = NULL;
				node->content.children[i]->content.children[1] = NULL;
				node->content.children[i]->content.children[2] = NULL;
				node->content.children[i]->content.children[3] = NULL;
			}
		}

		fillTreeNodes(tree, node->content.children[i]); // recursively fill the child nodes
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
			tree->free(node->content.data); // free this node's data
		}
	} else {
		for(int i = 0; i < 4; i++) {
			freeQuadtreeNode(tree, node->content.children[i]);
		}
	}

	// free the node if all the children are or the data is freed
	free(node);
}
