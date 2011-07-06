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
#include "modules/opengl/texture.h"
#include "modules/heightmap/heightmap.h"
#include "modules/quadtree/quadtree.h"

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
	/** The vertices to render */
	HeightmapVertex *vertices;
	/** The tiles to render */
	HeightmapTile *tiles;
	/** The quadtree from which to obtain the data */
	Quadtree *quadtree;
	/** The data prefix to prepend to file names on map loading */
	char *dataPrefix;
	/** The data suffix to append to file names on map loading */
	char *dataSuffix;
	/** The texture array with the height data */
	OpenGLTexture *heights;
	/** The texture array with the normal data */
	OpenGLTexture *normals;
	/** The OpenGL vertex buffer associated with this LOD map */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this LOD map */
	GLuint indexBuffer;
	/** The OpenGL primitive used to render the LOD map */
	OpenGLPrimitive primitive;
} OpenGLLodMap;

API OpenGLPrimitive *createOpenGLPrimitiveLodMap();
API void freeOpenGLPrimitiveLodMap(OpenGLPrimitive *primitive);

#endif