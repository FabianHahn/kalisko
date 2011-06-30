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

#include <math.h>
#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/scene/primitive.h"
#include "modules/opengl/primitive.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/opengl.h"
#include "modules/opengl/model.h"
#include "modules/opengl/material.h"
#include "modules/opengl/uniform.h"
#include "modules/opengl/texture.h"
#include "modules/linalg/Vector.h"
#include "modules/image/image.h"
#include "api.h"
#include "heightmap.h"
#include "scene.h"
#include "normals.h"

MODULE_NAME("heightmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL heightmaps");
MODULE_VERSION(0, 2, 13);
MODULE_BCVERSION(0, 2, 13);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("scene", 0, 5, 2), MODULE_DEPENDENCY("opengl", 0, 27, 0), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("image", 0, 5, 0));

MODULE_INIT
{
	return $(bool, scene, registerOpenGLPrimitiveSceneParser)("heightmap", &parseOpenGLScenePrimitiveHeightmap);
}

MODULE_FINALIZE
{
	$(bool, scene, unregisterOpenGLPrimitiveSceneParser)("heightmap");
}

/**
 * Creates a new OpenGL heightmap primitive
 *
 * @param heights			the image from which the heightmap values will be read (the primitive takes control over this value, do not free it yourself)
 * @result					the created OpenGL heightmap primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveHeightmap(Image *heights)
{
	OpenGLHeightmap *heightmap = ALLOCATE_OBJECT(OpenGLHeightmap);
	heightmap->vertices = ALLOCATE_OBJECTS(HeightmapVertex, heights->height * heights->width);
	heightmap->tiles = ALLOCATE_OBJECTS(HeightmapTile, (heights->height - 1) * (heights->width - 1));
	heightmap->heights = heights;
	heightmap->heightsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(heights);
	heightmap->normals = $(Image *, image, createImageFloat)(heights->width, heights->height, 3);
	heightmap->normalsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(heightmap->normals);
	heightmap->primitive.type = "heightmap";
	heightmap->primitive.data = heightmap;
	heightmap->primitive.setup_function = &setupOpenGLPrimitiveHeightmap;
	heightmap->primitive.draw_function = &drawOpenGLPrimitiveHeightmap;
	heightmap->primitive.update_function = NULL;
	heightmap->primitive.free_function = &freeOpenGLPrimitiveHeightmap;

	glGenBuffers(1, &heightmap->vertexBuffer);
	glGenBuffers(1, &heightmap->indexBuffer);
	initOpenGLPrimitiveHeightmap(&heightmap->primitive);
	synchronizeOpenGLPrimitiveHeightmap(&heightmap->primitive);

	if($(bool, opengl, checkOpenGLError)()) {
		freeOpenGLPrimitiveHeightmap(&heightmap->primitive);
		return NULL;
	}

	return &heightmap->primitive;
}

/**
 * Initlaizes an OpenGL heightmap primitive
 *
 * @param primitive			the OpenGL heightmap primitive to initialize
 * @result					true if successful
 */
API bool initOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to initialize OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;
	computeHeightmapNormals(heightmap->heights, heightmap->normals);

	if(!$(bool, opengl, synchronizeOpenGLTexture)(heightmap->normalsTexture)) {
		LOG_ERROR("Failed to initialize OpenGL heightmap: Could synchronize normals texture");
		return false;
	}

	for(unsigned int y = 0; y < heightmap->heights->height - 1; y++) {
		for(unsigned int x = 0; x < heightmap->heights->width - 1; x++) {
			int lowerLeft = x + y * heightmap->heights->width;
			int lowerRight = (x + 1) + y * heightmap->heights->width;
			int topLeft = x + (y + 1) * heightmap->heights->width;
			int topRight = (x + 1) + (y + 1) * heightmap->heights->width;

			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[0] = topLeft;
			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[1] = lowerRight;
			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[2] = lowerLeft;
			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[3] = topLeft;
			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[4] = topRight;
			heightmap->tiles[y * (heightmap->heights->width - 1) + x].indices[5] = lowerRight;
		}
	}

	for(unsigned int y = 0; y < heightmap->heights->height; y++) {
		for(unsigned int x = 0; x < heightmap->heights->width; x++) {
			heightmap->vertices[y * heightmap->heights->width + x].position[0] = x;
			heightmap->vertices[y * heightmap->heights->width + x].position[1] = y;
		}
	}

	return true;
}

/**
 * Sets up an OpenGL heightmap primitive for a model
 *
 * @param primitive			the OpenGL heightmap primitive to setup
 * @param model_name		the model name to setup the heightmap primitive for
 * @param mesh_name			the mesh name to setup the heightmap primitive for
 * @result					true if successful
 */
API bool setupOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, const char *model_name, const char *material_name)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to setup OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	// make sure the attached material has a texture array and add a count uniform for it
	OpenGLUniformAttachment *materialUniforms = $(OpenGLUniformAttachment *, opengl, getOpenGLMaterialUniforms)(material_name);
	OpenGLUniform *texture;

	if((texture = $(OpenGLUniform *, opengl, getOpenGLUniform)(materialUniforms, "texture")) == NULL || texture->type != OPENGL_UNIFORM_TEXTURE || texture->content.texture_value->type != OPENGL_TEXTURE_TYPE_2D_ARRAY) {
		LOG_ERROR("Failed to lookup OpenGL 'texture' uniform for heightmap primitive, expected 2D texture array");
		return false;
	}

	$(bool, opengl, detachOpenGLUniform)(materialUniforms, "textureCount");
	OpenGLUniform *textureCountUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(texture->content.texture_value->arraySize);
	$(bool, opengl, attachOpenGLUniform)(materialUniforms, "textureCount", textureCountUniform);

	// add model specific uniforms for the heightmap
	OpenGLHeightmap *heightmap = primitive->data;
	OpenGLUniformAttachment *uniforms = $(OpenGLUniformAttachment *, opengl, getOpenGLModelUniforms)(model_name);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heights");
	OpenGLUniform *heightsUniform = $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(heightmap->heightsTexture);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "heights", heightsUniform);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heightmapWidth");
	OpenGLUniform *heightmapWidthUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(heightmap->heights->width);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "heightmapWidth", heightmapWidthUniform);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heightmapHeight");
	OpenGLUniform *heightmapHeightUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(heightmap->heights->height);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "heightmapHeight", heightmapHeightUniform);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "normals");
	OpenGLUniform *normalsUniform = $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(heightmap->normalsTexture);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "normals", normalsUniform);

	return true;
}

/**
 * Returns the associated OpenGLHeightmap object for an OpenGL heightmap primitive
 *
 * @param primitive			the OpenGL heightmap primitive for which the OpenGLHeightmap object should be retrieved
 * @result					the OpenGLHeightmap object or NULL if the primitive is not an OpenGL particle effect primitive
 */
API OpenGLHeightmap *getOpenGLHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to retrieve OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	return primitive->data;
}

/**
 * Synchronizes a heightmap primitive with its associated OpenGL buffer objects
 *
 * @param primitive			the heightmap primitive to be synchronized
 * @result					true if successful
 */
API bool synchronizeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to synchronize OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, heightmap->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(HeightmapVertex) * heightmap->heights->height * heightmap->heights->width, heightmap->vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(HeightmapTile) * (heightmap->heights->height - 1) * (heightmap->heights->width - 1), heightmap->tiles, GL_STATIC_DRAW);

	if($(bool, opengl, checkOpenGLError)()) {
		return false;
	}

	return true;
}

/**
 * Draws an OpenGL heightmap primitive
 *
 * @param primitive			the heightmap primitive to draw
 */
API bool drawOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to draw OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, heightmap->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_UV, 2, GL_FLOAT, false, sizeof(HeightmapVertex), NULL + offsetof(HeightmapVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_UV);

	if($(bool, opengl, checkOpenGLError)()) {
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffer);
	glDrawElements(GL_TRIANGLES, (heightmap->heights->height - 1) * (heightmap->heights->width - 1) * 6, GL_UNSIGNED_INT, NULL);

	if($(bool, opengl, checkOpenGLError)()) {
		return false;
	}

	return true;
}

/**
 * Frees an OpenGL heightmap primitive
 *
 * @param primitive			the heightmap primitive to free
 */
API void freeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to free OpenGL heightmap: Primitive is not a heightmap");
		return;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	$(void, opengl, freeOpenGLTexture)(heightmap->heightsTexture);
	$(void, opengl, freeOpenGLTexture)(heightmap->normalsTexture);
	glDeleteBuffers(1, &heightmap->vertexBuffer);
	glDeleteBuffers(1, &heightmap->indexBuffer);
	free(heightmap->vertices);
	free(heightmap->tiles);
	free(heightmap);
}
