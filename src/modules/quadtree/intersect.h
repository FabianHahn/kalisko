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

#ifndef QUADTREE_INTERSECT_H
#define QUADTREE_INTERSECT_H

#include "modules/linalg/Vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "quadtree.h"

API bool intersectAABBSphere(Vector *pmin, Vector *pmax, Vector *position, double radius);

/**
 * Checks whether a quadtree node's 3D axis aligned bounding box intersects with a sphere. Note that this function returns will not work as expected when the sphere center is located inside the bounding box
 *
 * @param box			the quadtree node 3D bounding box to check for an intersection
 * @param position		the position of the sphere
 * @param radius		the radius of the sphere
 * @result				true if the sphere intersects the axis aligned bounding box
 */
static inline bool quadtreeAABB3DIntersectsSphere(QuadtreeAABB3D box, Vector *position, double radius)
{
	Vector *pmin = $(Vector *, linalg, createVector3)((double) box.minX, (double) box.minY, (double) box.minZ);
	Vector *pmax = $(Vector *, linalg, createVector3)((double) box.maxX, (double) box.maxY, (double) box.maxZ);
	bool ret = $(bool, quadtree, intersectAABBSphere)(pmin, pmax, position, radius);
	$(void, linalg, freeVector)(pmin);
	$(void, linalg, freeVector)(pmax);
	return ret;
}

#ifdef __cplusplus
}
#endif

#endif
