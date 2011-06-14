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

struct QuadtreeNodeStruct{
	/** The x position of the bottom left corner of the quadtree node */
	int x;
	/** The y position of the bottom left corner of the quadtree node */
	int y;
	/** The level of the quad tree node (level 0 means it's a leaf) */
	unsigned int level;
	/** Union containing either the child nodes or the leaf data */
	union {
		/** The child nodes of the quadtree node */
		struct QuadtreeNodeStruct *children[4];
		/** The leaf data of the quadtree node */
		void *data;
	} content;
};

typedef struct QuadtreeNodeStruct QuadtreeNode;

typedef struct {
	/** The root node of the quad tree */
	QuadtreeNode *root;
	/** The size of a leaf in the quad tree */
	unsigned int leafSize;
} Quadtree;

API Quadtree *createQuadtree(unsigned int leafSize);

/**
 * Checks whether a quadtree node is a leaf
 *
 * @param node		the quadtree node to check
 * @result			true if the node is indeed a leaf
 */
static inline bool quadtreeNodeIsLeaf(QuadtreeNode *node) {
	return node->level == 0;
}

/**
 * Returns the side length of the spanned square of the quadtree node
 *
 * @param tree		the quadtree to which the node belongs
 * @param node		the quadtree node to check
 * @result			the side length of the spanned square of the provided quadtree node
 */
static inline int quadtreeNodeSpan(Quadtree *tree, QuadtreeNode *node) {
	return tree->leafSize * (node->level + 1);
}

#endif
