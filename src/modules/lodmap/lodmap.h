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

#ifndef HEIGHTMAP_LODMAP_H
#define HEIGHTMAP_LODMAP_H

#include <GL/glew.h>
#include "modules/opengl/primitive.h"
#include "modules/opengl/model.h"
#include "modules/opengl/texture.h"
#include "modules/heightmap/heightmap.h"
#include "modules/quadtree/quadtree.h"
#include "modules/linalg/Vector.h"

/**
 * Struct representing an OpenGL LOD map tile
 */
typedef struct {
	/** The height field of the tile */
	Image *heights;
	/** The normal field of the tile */
	Image *normals;
} OpenGLLodMapTile;

/**
 * Struct representing an OpenGL LOD map
 */
typedef struct {
	/** The heightmap primitive used to render the LOD map */
	OpenGLPrimitive *heightmap;
	/** The quadtree from which to obtain the data */
	Quadtree *quadtree;
	/** The tile models used to render the LOD map */
	OpenGLModel **tileModels;
	/** The maximum viewing distance to be handled by this LOD map */
	unsigned int viewingDistance;
	/** The maximum number of tiles displayed simultaneously by the LOD map */
	unsigned int maxTiles;
	/** The data prefix to prepend to file names on map loading */
	char *dataPrefix;
	/** The data suffix to append to file names on map loading */
	char *dataSuffix;
} OpenGLLodMap;

API OpenGLLodMap *createOpenGLLodMap(unsigned int viewingDistance, unsigned int maxTiles, unsigned int leafSize, const char *dataPrefix, const char *dataSuffix);
API void updateOpenGLLodMap(OpenGLLodMap *lodmap, Vector *position);
API void drawOpenGLLodMap(OpenGLLodMap *lodmap);
API void freeOpenGLLodMap(OpenGLLodMap *lodmap);

/**
 * Returns the LOD range covered of an LOD level in a LOD map
 *
 * @param lodmap			the LOD map for which to retrieve the LOD range
 * @param level				the LOD level for which to retrieve the LOD range
 * @result					the LOD range for the specified level
 */
static unsigned int getLodMapRange(OpenGLLodMap *lodmap, unsigned int level)
{
	if(level > lodmap->quadtree->root->level) { // there can't be an LOD level higher than the tree's height, so just return the maximum viewing distance
		return lodmap->viewingDistance;
	}

	unsigned int diff = lodmap->quadtree->root->level - level;
	return (lodmap->viewingDistance << diff); // divide by two for each level below the max
}

#endif
