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

static inline Vector getHeightmapVector(Image *heights, unsigned int x, unsigned int y)
{
	return Vector3((float) x / (heights->width - 1.0f), getImage(heights, x, y, 1), (float) y / (heights->height - 1.0f));
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

	for(unsigned int y = 0; y < heightmap->heights->height; y++) {
		for(unsigned int x = 0; x < heightmap->heights->width; x++) {
			Vector normal = Vector3(0.0f, 0.0f, 0.0f);
			Vector current = getHeightmapVector(heightmap->heights, x, y);

			// Loop over neighbors to get contributions
			for(int j = -1; j <= 1; j++) {
				for(int i = -1; i <= 1; i++) {
					int yc = y + j;
					int xc = x + i;

					if(yc >= 0 && yc < (int) heightmap->heights->height && xc >= 0 && xc < (int) heightmap->heights->width) { // If valid, add difference to normal
						normal += (getHeightmapVector(heightmap->heights, xc, yc) - current);
					}
				}
			}

			if(normal * up < 0) { // If the normal is oriented towards the ground, flip it
				normal *= -1;
			}

			normal.normalize(); // Normalize it

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
