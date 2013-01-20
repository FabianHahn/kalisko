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

#include "dll.h"
#include "test.h"
#include "modules/quadtree/quadtree.h"
#define API

MODULE_NAME("test_quadtree");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the quadtree module");
MODULE_VERSION(0, 3, 1);
MODULE_BCVERSION(0, 3, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("quadtree", 0, 12, 2));

static void testDataLoadFunction(Quadtree *tree, QuadtreeNode *node);
static void testDataFreeFunction(Quadtree *tree, void *data);

TEST_CASE(expand);
TEST_CASE(data);

TEST_SUITE_BEGIN(quadtree)
	TEST_CASE_ADD(expand);
	TEST_CASE_ADD(data);
TEST_SUITE_END

TEST_CASE(expand)
{
	Quadtree *tree = createQuadtree(&testDataLoadFunction, &testDataFreeFunction);
	TEST_ASSERT(tree != NULL);
	TEST_ASSERT(!quadtreeContainsPoint(tree, 1.0, 1.0));

	QuadtreeNode *node = lookupQuadtreeNode(tree, 1.0, 1.0, 0);
	TEST_ASSERT(quadtreeContainsPoint(tree, 1.0, 1.0));
	TEST_ASSERT(quadtreeNodeContainsPoint(node, 1.0, 1.0));
	QuadtreeAABB box = quadtreeNodeAABB(node);
	TEST_ASSERT(box.minX == 1);
	TEST_ASSERT(box.maxX == 2);
	TEST_ASSERT(box.minY == 1);
	TEST_ASSERT(box.maxY == 2);
	TEST_ASSERT(tree->root->level == 1);
	TEST_ASSERT(tree->root->children[3] == node);
	QuadtreeAABB box1 = quadtreeNodeAABB(tree->root->children[1]);
	TEST_ASSERT(box1.minX == 1);
	TEST_ASSERT(box1.maxX == 2);
	TEST_ASSERT(box1.minY == 0);
	TEST_ASSERT(box1.maxY == 1);
	QuadtreeAABB box2 = quadtreeNodeAABB(tree->root->children[2]);
	TEST_ASSERT(box2.minX == 0);
	TEST_ASSERT(box2.maxX == 1);
	TEST_ASSERT(box2.minY == 1);
	TEST_ASSERT(box2.maxY == 2);

	QuadtreeNode *node2 = lookupQuadtreeNode(tree, 1.0, 1.0, 0);
	TEST_ASSERT(node == node2);

	TEST_ASSERT(!quadtreeContainsPoint(tree, -1.0, -1.0));
	QuadtreeNode *origRoot = tree->root;

	node = lookupQuadtreeNode(tree, -1.0, -1.0, 0);
	TEST_ASSERT(quadtreeContainsPoint(tree, -1.0, -1.0));
	TEST_ASSERT(quadtreeNodeContainsPoint(node, -1.0, -1.0));
	box = quadtreeNodeAABB(node);
	TEST_ASSERT(box.minX == -1);
	TEST_ASSERT(box.maxX == 0);
	TEST_ASSERT(box.minY == -1);
	TEST_ASSERT(box.maxY == 0);
	TEST_ASSERT(tree->root->level == 2);
	TEST_ASSERT(tree->root->children[3] == origRoot);
	QuadtreeAABB box0 = quadtreeNodeAABB(tree->root->children[0]);
	TEST_ASSERT(box0.minX == -2);
	TEST_ASSERT(box0.maxX == 0);
	TEST_ASSERT(box0.minY == -2);
	TEST_ASSERT(box0.maxY == 0);

	// cleanup
	freeQuadtree(tree);
}

TEST_CASE(data)
{
	Quadtree *tree = createQuadtree(&testDataLoadFunction, &testDataFreeFunction);
	TEST_ASSERT(tree != NULL);

	QuadtreeNode *data = lookupQuadtree(tree, 0.0, 0.0, 0);
	QuadtreeNode *node = lookupQuadtreeNode(tree, 0.0, 0.0, 0);
	TEST_ASSERT(data == node);

	data = lookupQuadtree(tree, 0.0, 1.0, 0);
	node = lookupQuadtreeNode(tree, 0.0, 1.0, 0);
	TEST_ASSERT(data == node);

	data = lookupQuadtree(tree, 0.0, 0.0, 0);
	node = lookupQuadtreeNode(tree, 0.0, 0.0, 0);
	TEST_ASSERT(data == node);

	// cleanup
	freeQuadtree(tree);
}

static void testDataLoadFunction(Quadtree *tree, QuadtreeNode *node)
{
	// store the node itself as data
	node->data = node;
}

static void testDataFreeFunction(Quadtree *tree, void *data)
{
	// don't do anything...
}
