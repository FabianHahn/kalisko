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
	/** The height texture of the tile */
	OpenGLTexture *heightsTexture;
	/** The normal field of the tile */
	Image *normals;
	/** The normals texture of the tile */
	OpenGLTexture *normalsTexture;
	/** The OpenGL model to render the tile */
	OpenGLModel *model;
	/** The OpenGL heightmap draw options to use to draw this tile */
	OpenGLHeightmapDrawOptions drawOptions;
} OpenGLLodMapTile;

/**
 * Struct representing an OpenGL LOD map
 */
typedef struct {
	/** The heightmap primitive used to render the LOD map */
	OpenGLPrimitive *heightmap;
	/** The quadtree from which to obtain the data */
	Quadtree *quadtree;
	/** The current selection of quadtree nodes to be rendered */
	GList *selection;
	/** The current viewer position */
	Vector *viewerPosition;
	/** The base viewing range covered by the lowest LOD level in the LOD map */
	float baseRange;
	/** The maximum viewing distance to be handled by this LOD map */
	unsigned int viewingDistance;
	/** The data prefix to prepend to file names on map loading */
	char *dataPrefix;
	/** The data suffix to append to file names on map loading */
	char *dataSuffix;
	/** The polygon rendering mode to use for the tile models */
	GLuint polygonMode;
	/** The factor of the LOD range at which the vertex morphing should start */
	float morphStartFactor;
} OpenGLLodMap;

API OpenGLLodMap *createOpenGLLodMap(double baseRange, unsigned int viewingDistance, unsigned int leafSize, const char *dataPrefix, const char *dataSuffix);
API void updateOpenGLLodMap(OpenGLLodMap *lodmap, Vector *position, bool autoExpand);
API void drawOpenGLLodMap(OpenGLLodMap *lodmap);
API void freeOpenGLLodMap(OpenGLLodMap *lodmap);

/**
 * Returns the LOD range covered of an LOD level in a LOD map
 *
 * @param lodmap			the LOD map for which to retrieve the LOD range
 * @param level				the LOD level for which to retrieve the LOD range
 * @result					the LOD range for the specified level
 */
static inline float getLodMapRange(OpenGLLodMap *lodmap, unsigned int level)
{
	unsigned int maxLevel = MAX(lodmap->viewingDistance, lodmap->quadtree->root->level);
	unsigned int lookupLevel = MIN(level, maxLevel);
	unsigned int scale = (1 << lookupLevel);

	return lodmap->baseRange * scale;
}

/**
 * Returns the LOD range covered by a quadtree node in the LOD map's quadtree
 *
 * @param lodmap			the LOD map for which to retrieve the LOD range
 * @param node				the quadtree node of the LOD map's quadtree for which to retrieve the LOD range
 * @result					the LOD range for the specified node
 */
static inline float getLodMapNodeRange(OpenGLLodMap *lodmap, QuadtreeNode *node)
{
	return getLodMapRange(lodmap, node->level);
}

#endif
