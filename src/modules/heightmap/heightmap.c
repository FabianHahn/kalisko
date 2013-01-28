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
#define API
#include "heightmap.h"
#include "scene.h"
#include "normals.h"

MODULE_NAME("heightmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL heightmaps");
MODULE_VERSION(0, 4, 4);
MODULE_BCVERSION(0, 4, 4);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("scene", 0, 8, 0), MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("image", 0, 5, 16));

static void fillHeightmapTile(HeightmapTile *tile, unsigned int heightmapWidth, unsigned int x, unsigned int y);

MODULE_INIT
{
	return registerOpenGLPrimitiveSceneParser("heightmap", &parseOpenGLScenePrimitiveHeightmap);
}

MODULE_FINALIZE
{
	unregisterOpenGLPrimitiveSceneParser("heightmap");
}

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
		heightmap->heightsTexture = createOpenGLVertexTexture2D(heights);
		heightmap->normals = createImageFloat(heights->width, heights->height, 3);
		heightmap->normalsTexture = createOpenGLVertexTexture2D(heightmap->normals);
	} else {
		heightmap->heightsTexture = NULL;
		heightmap->normals = NULL;
		heightmap->normalsTexture = NULL;
	}

	glGenBuffers(1, &heightmap->vertexBuffer);
	glGenBuffers(4, heightmap->indexBuffers);
	initOpenGLPrimitiveHeightmap(&heightmap->primitive);
	synchronizeOpenGLPrimitiveHeightmap(&heightmap->primitive);

	if(checkOpenGLError()) {
		freeOpenGLPrimitiveHeightmap(&heightmap->primitive);
		return NULL;
	}

	return &heightmap->primitive;
}

API bool initOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to initialize OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	if(heightmap->heights != NULL) { // there is a height field, so compute our normals and synchronize them
		computeHeightmapNormals(heightmap->heights, heightmap->normals, 1.0 / heightmap->width, 1.0 / heightmap->height);

		if(!synchronizeOpenGLTexture(heightmap->normalsTexture)) {
			logError("Failed to initialize OpenGL heightmap: Could synchronize normals texture");
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

API bool setupOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, OpenGLModel *model, const char *material)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to setup OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	if(heightmap->heights != NULL) { // only require fragment texture when we're not managed
		// make sure the attached material has a texture array and add a count uniform for it
		OpenGLUniformAttachment *materialUniforms = getOpenGLMaterialUniforms(material);
		OpenGLUniform *texture;

		if((texture = getOpenGLUniform(materialUniforms, "texture")) == NULL || texture->type != OPENGL_UNIFORM_TEXTURE || texture->content.texture_value->type != OPENGL_TEXTURE_TYPE_2D_ARRAY) {
			logError("Failed to lookup OpenGL 'texture' uniform for heightmap primitive, expected 2D texture array");
			return false;
		}

		detachOpenGLUniform(materialUniforms, "textureCount");
		OpenGLUniform *textureCountUniform = createOpenGLUniformInt(texture->content.texture_value->arraySize);
		attachOpenGLUniform(materialUniforms, "textureCount", textureCountUniform);
	}

	// add model specific uniforms for the heightmap
	OpenGLUniformAttachment *uniforms = model->uniforms;

	detachOpenGLUniform(uniforms, "heights");
	OpenGLUniform *heightsUniform = createOpenGLUniformTexture(heightmap->heightsTexture);
	attachOpenGLUniform(uniforms, "heights", heightsUniform);

	detachOpenGLUniform(uniforms, "heightmapWidth");
	OpenGLUniform *heightmapWidthUniform = createOpenGLUniformInt(heightmap->width);
	attachOpenGLUniform(uniforms, "heightmapWidth", heightmapWidthUniform);

	detachOpenGLUniform(uniforms, "heightmapHeight");
	OpenGLUniform *heightmapHeightUniform = createOpenGLUniformInt(heightmap->height);
	attachOpenGLUniform(uniforms, "heightmapHeight", heightmapHeightUniform);

	detachOpenGLUniform(uniforms, "normals");
	OpenGLUniform *normalsUniform = createOpenGLUniformTexture(heightmap->normalsTexture);
	attachOpenGLUniform(uniforms, "normals", normalsUniform);

	return true;
}

API OpenGLHeightmap *getOpenGLHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to retrieve OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	return primitive->data;
}

API bool synchronizeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to synchronize OpenGL heightmap: Primitive is not a heightmap");
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

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

API bool drawOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive, void *options_p)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to draw OpenGL heightmap: Primitive is not a heightmap");
		return false;
	}

	OpenGLHeightmap *heightmap = primitive->data;
	OpenGLHeightmapDrawOptions *options = options_p;

	glBindBuffer(GL_ARRAY_BUFFER, heightmap->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_UV, 2, GL_FLOAT, false, sizeof(HeightmapVertex), NULL + offsetof(HeightmapVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_UV);

	if(checkOpenGLError()) {
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

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

API void freeOpenGLPrimitiveHeightmap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "heightmap") != 0) {
		logError("Failed to free OpenGL heightmap: Primitive is not a heightmap");
		return;
	}

	OpenGLHeightmap *heightmap = primitive->data;

	if(heightmap->heights != NULL) { // only free the textures if we're not managed by elsewhere
		freeOpenGLTexture(heightmap->heightsTexture);
		freeOpenGLTexture(heightmap->normalsTexture);
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
