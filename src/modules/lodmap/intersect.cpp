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

#include <cassert>
#include "dll.h"
#include "modules/linalg/Vector.h"
extern "C" {
#include "modules/quadtree/quadtree.h"
}
#define API
extern "C" {
#include "lodmap.h"
}
#include "intersect.h"

static int intersectAABBSphere(Vector *pmin, Vector *pmax, Vector *position, double radius);

/**
 * Checks whether a quadtree node's 3D axis aligned bounding box intersects with a sphere. Make sure to update the node's weight after calling this function since it can cause the node to become loaded.
 * Note that this function returns will not work as expected when the sphere center is located inside the bounding box.
 *
 * @param tree			the quadtree to which the node belongs
 * @param node			the quadtree node which has to be intersected
 * @param position		the position of the sphere
 * @param radius		the radius of the sphere
 * @result				nonzero if the sphere intersects the axis aligned bounding box
 */
API int lodmapQuadtreeNodeIntersectsSphere(Quadtree *tree, QuadtreeNode *node, Vector *position, double radius)
{
	OpenGLLodMapTile *tile = (OpenGLLodMapTile *) node->data;
	float minY = tile->minHeight;
	float maxY = tile->maxHeight;

	float scale = quadtreeNodeScale(node);
	Vector pmin = Vector3(node->x, minY, node->y);
	Vector pmax = Vector3(node->x + scale, maxY, node->y + scale);
	return intersectAABBSphere(&pmin, &pmax, position, radius);
}

/**
 * Checks whether an axis aligned bounding box intersects with a sphere. Note that this function returns will not work as expected when the sphere center is located inside the bounding box
 *
 * @param pmin			the minimum position of the axis aligned bounding box
 * @param pmax			the maximum position of the axis aligned bounding box
 * @param position		the position of the sphere
 * @param radius		the radius of the sphere
 * @result				nonzero if the sphere intersects the axis aligned bounding box
 */
static int intersectAABBSphere(Vector *pmin, Vector *pmax, Vector *position, double radius)
{
	Vector boxPoint = Vector3(0.0f, 0.0f, 0.0f);
	const Vector& center = *position;
	const Vector& minPoint = *pmin;
	const Vector& maxPoint = *pmax;

	// Intersect for every axis individually to find the closest point on the box
	for(unsigned int k = 0; k < 3; k++) {
		if(center[k] < minPoint[k]) {
			boxPoint[k] = minPoint[k];
		} else if(center[k] > maxPoint[k]) {
			boxPoint[k] = maxPoint[k];
		} else {
			boxPoint[k] = center[k];
		}
	}

	Vector diff = center - boxPoint;
	float radius2 = radius * radius;

	if(diff.getLength2() < radius2) { // radius is larger than distance, so we have an intersection
		// LOG_DEBUG("Intersection: [%f,%f,%f]-[%f,%f,%f] with center [%f,%f,%f], r = %f", minPoint[0], minPoint[1], minPoint[2], maxPoint[0], maxPoint[1], maxPoint[2], (*position)[0], (*position)[1], (*position)[2], radius);
		return 1;
	} else {
		// LOG_DEBUG("No intersection: [%f,%f,%f]-[%f,%f,%f] with center [%f,%f,%f], r = %f", minPoint[0], minPoint[1], minPoint[2], maxPoint[0], maxPoint[1], maxPoint[2], (*position)[0], (*position)[1], (*position)[2], radius);
		return 0;
	}
}
