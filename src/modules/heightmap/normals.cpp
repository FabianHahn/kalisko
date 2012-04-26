/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
#include "modules/linalg/Matrix.h"
extern "C" {
#include "modules/image/image.h"
}
#define API

extern "C" {
#include "heightmap.h"
}
#include "normals.h"

static inline Vector getHeightmapVector(Image *heights, int x, int y)
{
	return Vector3((float) x, getImage(heights, x, y, 0), (float) y);
}

/**
 * Computes the heightmap normals for a heightfield image
 *
 * @param heights			the heightfield for which to compute the normal vectors
 * @param normals			the normal map in which to write the computed normals
 */
API void computeHeightmapNormals(Image *heights, Image *normals)
{
	assert(heights->height == normals->height);
	assert(heights->width == normals->width);
	assert(normals->channels >= 3);

	Vector up = Vector3(0.0f, 1.0f, 0.0f);
	int height = heights->height;
	int width = heights->width;

	for(int y = 0; y < height; y++) {
		int ym1 = y - 1 < 0 ? 0 : y - 1;
		int yp1 = y + 1 >= height ? height - 1 : y + 1;

		for(int x = 0; x < width; x++) {
			int xm1 = x - 1 < 0 ? 0 : x - 1;
			int xp1 = x + 1 >= width ? width - 1 : x + 1;

			Vector normal = Vector3(0.0f, 0.0f, 0.0f);
			Vector current = getHeightmapVector(heights, x, y);
			Vector exp1yp0 = getHeightmapVector(heights, xp1, y) - current;
			Vector exp1ym1 = getHeightmapVector(heights, xp1, ym1) - current;
			Vector exp0yp1 = getHeightmapVector(heights, x, yp1) - current;
			Vector exp0ym1 = getHeightmapVector(heights, x, ym1) - current;
			Vector exm1yp1 = getHeightmapVector(heights, xm1, yp1) - current;
			Vector exm1yp0 = getHeightmapVector(heights, xm1, y) - current;

			// add contributions from neighboring triangles
			normal += (exp1yp0 % exp1ym1).normalize();
			normal += (exp1ym1 % exp0ym1).normalize();
			normal += 2.0f * (exp0ym1 % exm1yp0).normalize();
			normal += (exm1yp0 % exm1yp1).normalize();
			normal += (exm1yp1 % exp0yp1).normalize();
			normal += 2.0f * (exp0yp1 % exp1yp0).normalize();
			normal.normalize(); // normalize it

			setImage(normals, x, y, 0, normal[0]);
			setImage(normals, x, y, 1, normal[1]);
			setImage(normals, x, y, 2, normal[2]);
		}
	}
}
