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
MODULE_VERSION(0, 4, 1);
MODULE_BCVERSION(0, 4, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("scene", 0, 8, 0), MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("image", 0, 5, 16));

static void fillHeightmapTile(HeightmapTile *tile, unsigned int heightmapWidth, unsigned int x, unsigned int y);

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
 * @param heights			the image from which the heightmap values will be read (the primitive takes control over this value, do not free it yourself) or NULL to create a heightmap that will be managed by e.g. another primitive
 * @param width				the width of the heightmap grid to create
 * @param height			the height of the heightmap grid to create
 * @result					the created OpenGL heightmap primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveHeightmap(Image *heights, unsigned int width, unsigned int height)
{
	unsigned int halfWidth = width / 2;
	unsigned int halfHeight = height / 2;

	OpenGLHeightmap *heightmap = ALLOCATE_OBJECT(OpenGLHeightmap);
	heightmap->vertices = ALLOCATE_OBJECTS(HeightmapVertex, width * height);
	heightmap->tiles[0] = ALLOCATE_OBJECTS(HeightmapTile, halfWidth * halfHeight);
	heightmap->tiles[1] = ALLOCATE_OBJECTS(HeightmapTile, (width - halfWidth - 1) * halfHeight);
	heightmap->tiles[2] = ALLOCATE_OBJECTS(HeightmapTile, halfWidth * (height - halfHeight - 1));
	heightmap->tiles[3] = ALLOCATE_OBJECTS(HeightmapTile, (width - halfWidth - 1) * (height - halfHeight - 1));
	heightmap->heights = heights;
	heightmap->width = width;
	heightmap->height = height;
	heightmap->primitive.type = "heightmap";
	heightmap->primitive.data = heightmap;
	heightmap->primitive.setup_function = &setupOpenGLPrimitiveHeightmap;
	heightmap->primitive.draw_function = &drawOpenGLPrimitiveHeightmap;
	heightmap->primitive.update_function = NULL;
	heightmap->primitive.free_function = &freeOpenGLPrimitiveHeightmap;

	if(heights != NULL) { // we have to manage ourselves, so create textures and a normal field
		heightmap->heightsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(heights);
		heightmap->normals = $(Image *, image, createImageFloat)(heights->width, heights->height, 3);
		heightmap->normalsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(heightmap->normals);
	} else {
		heightmap->heightsTexture = NULL;
		heightmap->normals = NULL;
		heightmap->normalsTexture = NULL;
	}

	glGenBuffers(1, &heightmap->vertexBuffer);
	glGenBuffers(4, heightmap->indexBuffers);
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

	if(heightmap->heights != NULL) { // there is a height field, so compute our normals and synchronize them
		computeHeightmapNormals(heightmap->heights, heightmap->normals);

		if(!$(bool, opengl, synchronizeOpenGLTexture)(heightmap->normalsTexture)) {
			LOG_ERROR("Failed to initialize OpenGL heightmap: Could synchronize normals texture");
			return false;
		}
	}

	// create indices
	unsigned int halfWidth = heightmap->width / 2;
	unsigned int halfHeight = heightmap->height / 2;
	unsigned int tileIndex = 0;

	// top left
	for(unsigned int y = 0; y < halfHeight; y++) {
		for(unsigned int x = 0; x < halfWidth; x++) {
			fillHeightmapTile(&heightmap->tiles[0][tileIndex++], heightmap->width, x, y);
		}
	}

	// top right
	tileIndex = 0;
	for(unsigned int y = 0; y < halfHeight; y++) {
		for(unsigned int x = halfWidth; x < heightmap->width - 1; x++) {
			fillHeightmapTile(&heightmap->tiles[1][tileIndex++], heightmap->width, x, y);
		}
	}

	// bottom left
	tileIndex = 0;
	for(unsigned int y = halfHeight; y < heightmap->height - 1; y++) {
		for(unsigned int x = 0; x < halfWidth; x++) {
			fillHeightmapTile(&heightmap->tiles[2][tileIndex++], heightmap->width, x, y);
		}
	}

	// top right
	tileIndex = 0;
	for(unsigned int y = halfHeight; y < heightmap->height - 1; y++) {
		for(unsigned int x = halfWidth; x < heightmap->width - 1; x++) {
			fillHeightmapTile(&heightmap->tiles[3][tileIndex++], heightmap->width, x, y);
		}
	}

	// create vertices
	for(unsigned int y = 0; y < heightmap->height; y++) {
		for(unsigned int x = 0; x < heightmap->width; x++) {
			heightmap->vertices[y * heightmap->width + x].position[0] = x;
			heightmap->vertices[y * heightmap->width + x].position[1] = y;
		}
	}

	return true;
}

/**
 * Sets up an OpenGL heightmap primitive for a model
 *
 * @param primitive			the OpenGL heightmap primitive to setup
 * @param model				the model to setup the heightmap primitive for
 * @param material			the material name to setup the heightmap primitive for
 * @result					true if successful
 */
API bool setupOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, OpenGLModel *model, const char *material)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to setup OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	if(heightmap->heights != NULL) { // only require fragment texture when we're not managed
		// make sure the attached material has a texture array and add a count uniform for it
		OpenGLUniformAttachment *materialUniforms = $(OpenGLUniformAttachment *, opengl, getOpenGLMaterialUniforms)(material);
		OpenGLUniform *texture;

		if((texture = $(OpenGLUniform *, opengl, getOpenGLUniform)(materialUniforms, "texture")) == NULL || texture->type != OPENGL_UNIFORM_TEXTURE || texture->content.texture_value->type != OPENGL_TEXTURE_TYPE_2D_ARRAY) {
			LOG_ERROR("Failed to lookup OpenGL 'texture' uniform for heightmap primitive, expected 2D texture array");
			return false;
		}

		$(bool, opengl, detachOpenGLUniform)(materialUniforms, "textureCount");
		OpenGLUniform *textureCountUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(texture->content.texture_value->arraySize);
		$(bool, opengl, attachOpenGLUniform)(materialUniforms, "textureCount", textureCountUniform);
	}

	// add model specific uniforms for the heightmap
	OpenGLUniformAttachment *uniforms = model->uniforms;

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heights");
	OpenGLUniform *heightsUniform = $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(heightmap->heightsTexture);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "heights", heightsUniform);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heightmapWidth");
	OpenGLUniform *heightmapWidthUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(heightmap->width);
	$(bool, opengl, attachOpenGLUniform)(uniforms, "heightmapWidth", heightmapWidthUniform);

	$(bool, opengl, detachOpenGLUniform)(uniforms, "heightmapHeight");
	OpenGLUniform *heightmapHeightUniform = $(OpenGLUniform *, opengl, createOpenGLUniformInt)(heightmap->height);
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
 * @result					the OpenGLHeightmap object or NULL if the primitive is not an OpenGL heightmap primitive
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

	// synchronize vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, heightmap->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(HeightmapVertex) * heightmap->height * heightmap->width, heightmap->vertices, GL_STATIC_DRAW);

	// synchronize index buffers
	unsigned int halfWidth = heightmap->width / 2;
	unsigned int halfHeight = heightmap->height / 2;

	// top left
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(HeightmapTile) * halfWidth * halfHeight, heightmap->tiles[0], GL_STATIC_DRAW);

	// top left
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(HeightmapTile) * (heightmap->width - halfWidth - 1) * halfHeight, heightmap->tiles[1], GL_STATIC_DRAW);

	// top left
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(HeightmapTile) * halfWidth * (heightmap->height - halfHeight - 1), heightmap->tiles[2], GL_STATIC_DRAW);

	// top left
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(HeightmapTile) * (heightmap->width - halfWidth - 1) * (heightmap->height - halfHeight - 1), heightmap->tiles[3], GL_STATIC_DRAW);

	if($(bool, opengl, checkOpenGLError)()) {
		return false;
	}

	return true;
}

/**
 * Draws an OpenGL heightmap primitive
 *
 * @param primitive			the heightmap primitive to draw
 * @param options_p			a pointer to custom options to be considered for this draw call
 */
API bool drawOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, void *options_p)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		LOG_ERROR("Failed to draw OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;
	OpenGLHeightmapDrawOptions *options = options_p;

	glBindBuffer(GL_ARRAY_BUFFER, heightmap->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_UV, 2, GL_FLOAT, false, sizeof(HeightmapVertex), NULL + offsetof(HeightmapVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_UV);

	if($(bool, opengl, checkOpenGLError)()) {
		return false;
	}

	unsigned int halfWidth = heightmap->width / 2;
	unsigned int halfHeight = heightmap->height / 2;

	// top left
	if(options == NULL || options->drawMode & OPENGL_HEIGHTMAP_DRAW_TOP_LEFT) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[0]);
		glDrawElements(GL_TRIANGLES, halfWidth * halfHeight * 6, GL_UNSIGNED_INT, NULL);
	}

	// top right
	if(options == NULL || options->drawMode & OPENGL_HEIGHTMAP_DRAW_TOP_RIGHT) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[1]);
		glDrawElements(GL_TRIANGLES, (heightmap->width - halfWidth - 1) * halfHeight * 6, GL_UNSIGNED_INT, NULL);
	}

	// bottom left
	if(options == NULL || options->drawMode & OPENGL_HEIGHTMAP_DRAW_BOTTOM_LEFT) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[2]);
		glDrawElements(GL_TRIANGLES, halfWidth * (heightmap->height - halfHeight - 1) * 6, GL_UNSIGNED_INT, NULL);
	}

	// bottom right
	if(options == NULL || options->drawMode & OPENGL_HEIGHTMAP_DRAW_BOTTOM_RIGHT) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heightmap->indexBuffers[3]);
		glDrawElements(GL_TRIANGLES, (heightmap->width - halfWidth - 1) * (heightmap->height - halfHeight - 1) * 6, GL_UNSIGNED_INT, NULL);
	}

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

	if(heightmap->heights != NULL) { // only free the textures if we're not managed by elsewhere
		$(void, opengl, freeOpenGLTexture)(heightmap->heightsTexture);
		$(void, opengl, freeOpenGLTexture)(heightmap->normalsTexture);
	}

	glDeleteBuffers(1, &heightmap->vertexBuffer);
	glDeleteBuffers(4, heightmap->indexBuffers);
	free(heightmap->vertices);
	free(heightmap->tiles[0]);
	free(heightmap->tiles[1]);
	free(heightmap->tiles[2]);
	free(heightmap->tiles[3]);
	free(heightmap);
}

/**
 * Fills a heightmap tile with index buffer data
 *
 * @param tile				the tile to fill
 * @param heightmapWidth	the width of the heightmap for which to fill the tile
 * @param x					the x coordinate of the tile
 * @param y					the y coordinate of the tile
 */
static void fillHeightmapTile(HeightmapTile *tile, unsigned int heightmapWidth, unsigned int x, unsigned int y)
{
	int lowerLeft = x + y * heightmapWidth;
	int lowerRight = (x + 1) + y * heightmapWidth;
	int topLeft = x + (y + 1) * heightmapWidth;
	int topRight = (x + 1) + (y + 1) * heightmapWidth;

	tile->indices[0] = topLeft;
	tile->indices[1] = lowerRight;
	tile->indices[2] = lowerLeft;
	tile->indices[3] = topLeft;
	tile->indices[4] = topRight;
	tile->indices[5] = lowerRight;
}
