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
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, QuadtreeNode *node, double x, double y);

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
 * Lookup a leaf node in the quadtree
 *
 * @param tree			the quadtree to lookup
 * @param x				the x coordinate to lookup
 * @param y				the y coordinate to lookup
 * @result				the looked up quadtree node
 */
API QuadtreeNode *lookupQuadtree(Quadtree *tree, double x, double y)
{
	return lookupQuadtreeNode(tree, tree->root, x, y);
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
static QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, QuadtreeNode *node, double x, double y)
{
	if(quadtreeNodeContainsPoint(tree, node, x, y)) {
		if(quadtreeNodeIsLeaf(node)) {
			return node;
		} else {
			int index = quadtreeNodeGetContainingChildIndex(tree, node, x, y);
			QuadtreeNode *child = node->content.children[index];
			return lookupQuadtreeNode(tree, child, x, y);
		}
	}

	return NULL;
}
