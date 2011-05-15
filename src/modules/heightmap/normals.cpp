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
#include "modules/linalg/Matrix.h"
extern "C" {
#include "modules/image/image.h"
#include "modules/opengl/primitive.h"
#include "modules/opengl/texture.h"
}
#include "api.h"

extern "C" {
#include "heightmap.h"
}
#include "normals.h"

static inline Vector getHeightmapVector(Image *heights, int x, int y)
{
	return Vector3((float) x / (heights->width - 1.0f), getImage(heights, x, y, 0), (float) y / (heights->height - 1.0f));
}

/**
 * Computes the heightmap normals for an OpenGL heightmap primitive and synchronizes them to their texture
 *
 * @param primitive		the OpenGL heightmap primitive to compute the normals for
 * @result				true if successful
 */
API bool computeOpenGLPrimitiveHeightmapNormals(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to compute OpenGL heightmap normals: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = (OpenGLHeightmap *) primitive->data;

	Vector up = Vector3(0.0f, 1.0f, 0.0f);
	int height = heightmap->heights->height;
	int width = heightmap->heights->width;

	for(int y = 0; y < height; y++) {
		int ym1 = y - 1 < 0 ? 0 : y - 1;
		int yp1 = y + 1 >= height ? height - 1 : y + 1;

		for(int x = 0; x < width; x++) {
			int xm1 = x - 1 < 0 ? 0 : x - 1;
			int xp1 = x + 1 >= width ? width - 1 : x + 1;

			Vector normal = Vector3(0.0f, 0.0f, 0.0f);
			Vector current = getHeightmapVector(heightmap->heights, x, y);
			Vector exp1yp0 = getHeightmapVector(heightmap->heights, xp1, y) - current;
			Vector exp1ym1 = getHeightmapVector(heightmap->heights, xp1, ym1) - current;
			Vector exp0yp1 = getHeightmapVector(heightmap->heights, x, yp1) - current;
			Vector exp0ym1 = getHeightmapVector(heightmap->heights, x, ym1) - current;
			Vector exm1yp1 = getHeightmapVector(heightmap->heights, xm1, yp1) - current;
			Vector exm1yp0 = getHeightmapVector(heightmap->heights, xm1, y) - current;

			// add contributions from neighboring triangles
			normal += (exp1yp0 % exp1ym1).normalize();
			normal += (exp1ym1 % exp0ym1).normalize();
			normal += 2.0f * (exp0ym1 % exm1yp0).normalize();
			normal += (exm1yp0 % exm1yp1).normalize();
			normal += (exm1yp1 % exp0yp1).normalize();
			normal += 2.0f * (exp0yp1 % exp1yp0).normalize();
			normal.normalize(); // normalize it

			setImage(heightmap->normals, x, y, 0, normal[0]);
			setImage(heightmap->normals, x, y, 1, normal[1]);
			setImage(heightmap->normals, x, y, 2, normal[2]);
		}
	}

	if(!$(bool, opengl, synchronizeOpenGLTexture)(heightmap->normalsTexture)) {
		return false;
	}

	return true;
}
