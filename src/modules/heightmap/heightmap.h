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

#ifndef HEIGHTMAP_HEIGHTMAP_H
#define HEIGHTMAP_HEIGHTMAP_H

#include <GL/glew.h>
#include "modules/opengl/primitive.h"
#include "modules/opengl/model.h"
#include "modules/opengl/texture.h"
#include "modules/linalg/Vector.h"
#include "modules/image/image.h"

/**
 * Struct representing a heightmap vertex
 */
typedef struct {
	/** The position of the vertex in the heightmap */
	float position[2];
} HeightmapVertex;

/**
 * Struct representing a heightmap tile
 */
typedef struct {
	/** The indices of the heightmap tile */
	unsigned int indices[6];
} HeightmapTile;

/**
 * Struct representing an OpenGL heightmap
 */
typedef struct {
	/** The width of the heightmap */
	unsigned int width;
	/** The height of the heightmap */
	unsigned int height;
	/** The vertices to render */
	HeightmapVertex *vertices;
	/** The tiles to render */
	HeightmapTile *tiles;
	/** The image with the height data */
	Image *heights;
	/** The texture with the height data */
	OpenGLTexture *heightsTexture;
	/** The image with the normal data */
	Image *normals;
	/** The texture with the normal data */
	OpenGLTexture *normalsTexture;
	/** The OpenGL vertex buffer associated with this heightmap */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this heightmap */
	GLuint indexBuffer;
	/** The OpenGL primitive used to render the heightmap */
	OpenGLPrimitive primitive;
} OpenGLHeightmap;

API OpenGLPrimitive *createOpenGLPrimitiveHeightmap(Image *heights, unsigned int width, unsigned int height);
API bool initOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive);
API bool setupOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, OpenGLModel *model, const char *material);
API OpenGLHeightmap *getOpenGLHeightmap(OpenGLPrimitive *primitive);
API bool synchronizeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive);
API bool drawOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive);
API void freeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive);

#endif
