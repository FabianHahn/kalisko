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
#include "api.h"

MODULE_NAME("test_quadtree");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the quadtree module");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("quadtree", 0, 3, 0));

typedef struct {
	Quadtree *tree;
	double x;
	double y;
} TestData;

static void *testDataLoadFunction(Quadtree *tree, double x, double y);
static void testDataFreeFunction(Quadtree *tree, void *data);

TEST_CASE(expand);
TEST_CASE(data);

TEST_SUITE_BEGIN(quadtree)
	TEST_CASE_ADD(expand);
	TEST_CASE_ADD(data);
TEST_SUITE_END

TEST_CASE(expand)
{
	Quadtree *tree = $(Quadtree *, quadtree, createQuadtree)(1.0, NULL, NULL);
	QuadtreeNode *origRoot = tree->root;
	TEST_ASSERT(tree != NULL);
	TEST_ASSERT(!quadtreeContainsPoint(tree, 1.0, 1.0));

	QuadtreeNode *node = $(QuadtreeNode *, quadtree, lookupQuadtreeNode)(tree, 1.0, 1.0);
	TEST_ASSERT(quadtreeContainsPoint(tree, 1.0, 1.0));
	TEST_ASSERT(quadtreeNodeContainsPoint(tree, node, 1.0, 1.0));
	TEST_ASSERT(node->x == 1.0);
	TEST_ASSERT(node->y == 1.0);
	TEST_ASSERT(tree->root->level == 1);
	TEST_ASSERT(tree->root->content.children[0] == origRoot);
	TEST_ASSERT(tree->root->content.children[3] == node);
	TEST_ASSERT(tree->root->content.children[1]->x == 1.0);
	TEST_ASSERT(tree->root->content.children[1]->y == 0.0);
	TEST_ASSERT(tree->root->content.children[2]->x == 0.0);
	TEST_ASSERT(tree->root->content.children[2]->y == 1.0);

	QuadtreeNode *node2 = $(QuadtreeNode *, quadtree, lookupQuadtreeNode)(tree, 1.0, 1.0);
	TEST_ASSERT(node == node2);

	TEST_ASSERT(!quadtreeContainsPoint(tree, -1.0, -1.0));
	origRoot = tree->root;

	node = $(QuadtreeNode *, quadtree, lookupQuadtreeNode)(tree, -1.0, -1.0);
	TEST_ASSERT(quadtreeContainsPoint(tree, -1.0, -1.0));
	TEST_ASSERT(quadtreeNodeContainsPoint(tree, node, -1.0, -1.0));
	TEST_ASSERT(node->x == -1.0);
	TEST_ASSERT(node->y == -1.0);
	TEST_ASSERT(tree->root->level == 2);
	TEST_ASSERT(tree->root->content.children[3] == origRoot);
	TEST_ASSERT(tree->root->content.children[0]->x == -2.0);
	TEST_ASSERT(tree->root->content.children[0]->y == -2.0);

	$(void, quadtree, freeQuadtree)(tree);

	TEST_PASS;
}

TEST_CASE(data)
{
	Quadtree *tree = $(Quadtree *, quadtree, createQuadtree)(1.0, &testDataLoadFunction, &testDataFreeFunction);
	TEST_ASSERT(tree != NULL);

	TestData *data = $(void *, quadtree, lookupQuadtree)(tree, 0.0, 0.0);
	TEST_ASSERT(data->tree == tree);
	TEST_ASSERT(data->x == 0.0);
	TEST_ASSERT(data->y == 0.0);

	data = $(void *, quadtree, lookupQuadtree)(tree, 0.0, 1.0);
	TEST_ASSERT(data->tree == tree);
	TEST_ASSERT(data->x == 0.0);
	TEST_ASSERT(data->y == 1.0);

	data = $(void *, quadtree, lookupQuadtree)(tree, 0.0, 0.0);
	TEST_ASSERT(data->tree == tree);
	TEST_ASSERT(data->x == 0.0);
	TEST_ASSERT(data->y == 0.0);

	$(void, quadtree, freeQuadtree)(tree);

	TEST_PASS;
}

static void *testDataLoadFunction(Quadtree *tree, double x, double y)
{
	// store values as they're passed
	TestData *data = ALLOCATE_OBJECT(TestData);
	data->tree = tree;
	data->x = x;
	data->y = y;
	return data;
}

static void testDataFreeFunction(Quadtree *tree, void *data)
{
	free(data); // just free the struct
}
