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

#ifndef QUADTREE_QUADTREE_H
#define QUADTREE_QUADTREE_H

#include <math.h>
#include <assert.h>

/**
 * Quadtree node struct
 */
struct QuadtreeNodeStruct{
	/** The x position of the bottom left corner of the quadtree node */
	int x;
	/** The y position of the bottom left corner of the quadtree node */
	int y;
	/** The level of the quad tree node (level 0 means it's a leaf) */
	unsigned short level;
	/** The weight of the node, i.e. how much data is loaded in its children */
	unsigned short weight;
	/** The last access time (used for caching) */
	double time;
	/** The child nodes of the quadtree node */
	struct QuadtreeNodeStruct *children[4];
	/** The data of the quadtree node */
	void *data;
};

typedef struct QuadtreeNodeStruct QuadtreeNode;

struct QuadtreeStruct; // forward declaration

typedef void *(QuadtreeDataLoadFunction)(struct QuadtreeStruct *tree, QuadtreeNode *node);
typedef void (QuadtreeDataFreeFunction)(struct QuadtreeStruct *tree, void *data);

/**
 * Quadtree struct
 */
struct QuadtreeStruct {
	/** The root node of the quad tree */
	QuadtreeNode *root;
	/** The size of a leaf in the quad tree */
	unsigned short leafSize;
	/** The capacity of the quadtree's data cache */
	unsigned int capacity;
	/** The loader function for quadtree data */
	QuadtreeDataLoadFunction *load;
	/** The free function for quadtree data */
	QuadtreeDataFreeFunction *free;
	/** Specifies whether the load function expects its child nodes to be already loaded */
	bool preloadChildData;
	/** The usage factor to which prune will reduce the quadtree's data cache */
	float pruneFactor;
};

typedef struct QuadtreeStruct Quadtree;

/**
 * Struct for 2D axis aligned bounding boxes used for quadtrees
 */
typedef struct {
	/** The minimum X coordinate of the bounding box */
	int minX;
	/** The minimum Y coordinate of the bounding box */
	int minY;
	/** The maximum X coordinate of the bounding box */
	int maxX;
	/** The maximum Y coordinate of the bounding box */
	int maxY;
} QuadtreeAABB;

API Quadtree *createQuadtree(unsigned int leafSize, unsigned int capacity, QuadtreeDataLoadFunction *load, QuadtreeDataFreeFunction *free, bool preloadChildData);
API void expandQuadtree(Quadtree *tree, double x, double y);
API void *lookupQuadtree(Quadtree *tree, double x, double y, unsigned int level);
API QuadtreeNode *lookupQuadtreeNode(Quadtree *tree, double x, double y, unsigned int level);
API void pruneQuadtree(Quadtree *tree);
API char *dumpQuadtree(Quadtree *tree);
API void freeQuadtree(Quadtree *tree);

/**
 * Checks whether a quadtree node is a leaf
 *
 * @param node		the quadtree node to check
 * @result			true if the node is indeed a leaf
 */
static inline bool quadtreeNodeIsLeaf(QuadtreeNode *node)
{
	return node->level == 0;
}

/**
 * Checks whether a quadtree node's data is loaded
 *
 * @param node		the quadtree node to check
 * @result			true if the node data is loaded
 */
static inline bool quadtreeNodeDataIsLoaded(QuadtreeNode *node)
{
	return node->data != NULL;
}

/**
 * Returns the world scale of a quadtree node determined by its level
 *
 * @param node		the quadtree node of which to determine the scale
 * @result			the world scale of the quadtree node
 */
static inline unsigned int quadtreeNodeScale(QuadtreeNode *node)
{
	return (1 << node->level);
}

/**
 * Returns the side length of the spanned square of a quadtree node
 *
 * @param tree		the quadtree to which the node belongs
 * @param node		the quadtree node to check
 * @result			the side length of the spanned square of the provided quadtree node
 */
static inline unsigned int quadtreeNodeSpan(Quadtree *tree, QuadtreeNode *node)
{
	unsigned int scale = quadtreeNodeScale(node);
	return tree->leafSize * scale;
}

/**
 * Returns the 2D axis aligned bounding box of the spanned square of a quadtree node
 *
 * @param tree		the quadtree to which the node belongs
 * @param node		the quadtree node to check
 * @result			the 2D axis aligned bounding box of the spanned square of the provided quadtree node
 */
static inline QuadtreeAABB quadtreeNodeAABB(Quadtree *tree, QuadtreeNode *node)
{
	unsigned int span = quadtreeNodeSpan(tree, node);

	QuadtreeAABB box;
	box.minX = node->x;
	box.maxX = node->x + span;
	box.minY = node->y;
	box.maxY = node->y + span;

	return box;
}

/**
 * Checks whether a quadtree node contains a point
 *
 * @param tree		the quadtree to which the node belongs
 * @param node		the quadtree node to check
 * @param x			the x coordinate of the point to check
 * @param y			the y coordinate of the point to check
 * @result			true if the quadtree node contains the point
 */
static inline bool quadtreeNodeContainsPoint(Quadtree *tree, QuadtreeNode *node, double x, double y)
{
	QuadtreeAABB box = quadtreeNodeAABB(tree, node);
	return x >= box.minX && x < box.maxX && y >= box.minY && y < box.maxY;
}

/**
 * Checks whether a quadtree contains a point
 *
 * @param tree		the quadtree to which the node belongs
 * @param x			the x coordinate of the point to check
 * @param y			the y coordinate of the point to check
 * @result			true if the quadtree contains the point
 */
static inline bool quadtreeContainsPoint(Quadtree *tree, double x, double y)
{
	return quadtreeNodeContainsPoint(tree, tree->root, x, y);
}

/**
 * Retrieves the child index of the node that contains a specified point
 *
 * @param tree		the quadtree to which the node belongs
 * @param node		the quadtree node to check
 * @param x			the x coordinate of the point to check
 * @param y			the y coordinate of the point to check
 * @result			the index of the child node
 */
static inline int quadtreeNodeGetContainingChildIndex(Quadtree *tree, QuadtreeNode *node, double x, double y)
{
	assert(quadtreeNodeContainsPoint(tree, node, x, y));

	unsigned int span = quadtreeNodeSpan(tree, node);
	int halfspan = span / 2;
	QuadtreeAABB box = quadtreeNodeAABB(tree, node);
	bool isLowerX = x < (box.minX + halfspan);
	bool isLowerY = y < (box.minY + halfspan);

	return (isLowerX ? 0 : 1) + (isLowerY ? 0 : 2);
}

#endif
