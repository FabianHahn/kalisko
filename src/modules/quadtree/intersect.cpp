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

#include "dll.h"
#include "modules/linalg/Vector.h"
#include "api.h"
extern "C" {
#include "quadtree.h"
}
#include "intersect.h"

/**
 * Checks whether a quadtree bounding box intersects with a sphere. Note that this function returns will not work as expected when the sphere center is located inside the bounding box
 *
 * @param tree			the quadtree to which the bounding box belongs
 * @param box			the axis aligned bounding box to check
 * @param position		the position of the sphere
 * @param radius		the radius of the sphere
 * @result				true if the sphere intersects the axis aligned bounding box
 */
API bool quadtreeAABB3DIntersectsSphere(Quadtree *tree, QuadtreeAABB3D box, Vector *position, double radius)
{
	Vector boxPoint = Vector3(0.0f, 0.0f, 0.0f);
	const Vector& center = *position;

	// Intersect for every axis individually to find the closest point on the box
	if(center[0] < box.minX) {
		boxPoint[0] = box.minX;
	} else if(center[0] > box.maxX) {
		boxPoint[0] = box.maxX;
	} else {
		boxPoint[0] = center[0];
	}

	if(center[1] < box.minY) {
		boxPoint[1] = box.minY;
	} else if(center[1] > box.maxY) {
		boxPoint[1] = box.maxY;
	} else {
		boxPoint[1] = center[1];
	}

	if(center[2] < box.minZ) {
		boxPoint[2] = box.minZ;
	} else if(center[2] > box.maxZ) {
		boxPoint[2] = box.maxZ;
	} else {
		boxPoint[2] = center[2];
	}

	Vector diff = center - boxPoint;
	float radius2 = radius * radius;

	if(diff.getLength2() < radius2) { // radius is larger than distance, so we have an intersection
		return true;
	} else {
		return false;
	}
}
