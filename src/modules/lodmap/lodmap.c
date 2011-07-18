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
#include "modules/quadtree/quadtree.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#include "modules/heightmap/normals.h"
#include "modules/opengl/material.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "lodmap.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 1, 13);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 4), MODULE_DEPENDENCY("heightmap", 0, 3, 2), MODULE_DEPENDENCY("quadtree", 0, 7, 6), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 1, 4), MODULE_DEPENDENCY("linalg", 0, 3, 4));

static void *loadLodMapTile(Quadtree *tree, QuadtreeNode *node);
static void freeLodMapTile(Quadtree *tree, void *data);

/**
 * A hash table associating quadtree objects with their parent LOD map primitives
 */
static GHashTable *maps;

MODULE_INIT
{
	maps = g_hash_table_new(&g_direct_hash, &g_direct_equal);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(maps);
}

/**
 * Creates an OpenGL LOD map
 *
 * @param maxTiles		the maximum number of tiles displayed simultaneously by the LOD map
 * @param leafSize		the leaf size of the LOD map
 * @param dataPrefix	the prefix that should be prepended to all loaded tiles
 * @param dataSuffix	the suffix that should be appended to all loaded tiles
 * @result				the created OpenGL LOD map
 */
API OpenGLLodMap *createOpenGLLodMap(unsigned int maxTiles, unsigned int leafSize, const char *dataPrefix, const char *dataSuffix)
{
	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->heightmap = $(OpenGLPrimitive *, heightmap, createOpenGLPrimitiveHeightmap)(NULL, leafSize, leafSize); // create a managed heightmap that will serve as instance for our rendered tiles
	lodmap->quadtree = $(Quadtree *, quadtree, createQuadtree)(leafSize, 25, &loadLodMapTile, &freeLodMapTile, true);
	lodmap->tileModels = ALLOCATE_OBJECTS(OpenGLModel *, maxTiles);
	lodmap->maxTiles = maxTiles;
	lodmap->dataPrefix = strdup(dataPrefix);
	lodmap->dataSuffix = strdup(dataSuffix);

	// create lodmap material
	$(bool, opengl, deleteOpenGLMaterial)("lodmap");
	char *execpath = $$(char *, getExecutablePath());
	GString *vertexShaderPath = g_string_new(execpath);
	g_string_append_printf(vertexShaderPath, "/modules/lodmap/lodmap.glslv");
	GString *fragmentShaderPath = g_string_new(execpath);
	g_string_append_printf(fragmentShaderPath, "/modules/lodmap/lodmap.glslf");

	bool result = $(bool, opengl, createOpenGLMaterialFromFiles)("lodmap", vertexShaderPath->str, fragmentShaderPath->str);

	g_string_free(vertexShaderPath, true);
	g_string_free(fragmentShaderPath, true);
	free(execpath);

	// initialize tile models
	for(unsigned int i = 0; i < maxTiles; i++) {
		lodmap->tileModels[i] = $(OpenGLModel *, opengl, createOpenGLModel)(lodmap->heightmap);
		result = result && $(bool, opengl, attachOpenGLModelMaterial)(lodmap->tileModels[i], "lodmap");
	}

	g_hash_table_insert(maps, lodmap->quadtree, lodmap);

	if(!result) {
		freeOpenGLLodMap(lodmap);
		return NULL;
	}

	return lodmap;
}

/**
 * Updates an OpenGL LOD map
 *
 * @param lodmap		the LOD map to update
 * @param position		the viewer position with respect to which the LOD map should be updated
 */
API void updateOpenGLLodMap(OpenGLLodMap *lodmap, Vector *position)
{
	// reset all models
	for(unsigned int i = 0; i < lodmap->maxTiles; i++) {
		OpenGLModel *model = lodmap->tileModels[i];
		model->visible = false;
	}
}

/**
 * Draws an OpenGL LOD map
 *
 * @param lodmap		the LOD map to draw
 */
API void drawOpenGLLodMap(OpenGLLodMap *lodmap)
{
	// draw all visible models
	for(unsigned int i = 0; i < lodmap->maxTiles; i++) {
		OpenGLModel *model = lodmap->tileModels[i];
		if(model->visible) {
			if(!$(bool, opengl, drawOpenGLModel)(model)) {
				LOG_WARNING("Failed to draw OpenGL LOD map tile model %u", i);
			}
		}
	}
}

/**
 * Frees an OpenGL LOD map
 *
 * @param lodmap				the OpenGL LOD map to free
 */
API void freeOpenGLLodMap(OpenGLLodMap *lodmap)
{
	g_hash_table_remove(maps, lodmap->quadtree);
	$(void, quadtree, freeQuadtree)(lodmap->quadtree);

	// free tile models
	for(unsigned int i = 0; i < lodmap->maxTiles; i++) {
		$(void, opengl, freeOpenGLModel)(lodmap->tileModels[i]);
	}

	$(bool, opengl, deleteOpenGLMaterial)("lodmap");
	free(lodmap->tileModels);
	freeOpenGLPrimitive(lodmap->heightmap);
	free(lodmap->dataPrefix);
	free(lodmap->dataSuffix);
	free(lodmap);
}

/**
 * Loads an LOD map tile
 *
 * @param tree			the quadtree for which to load the map tile
 * @param node			the quadtree node for which to load the map data
 * @result				the loaded LOD map tile
 */
static void *loadLodMapTile(Quadtree *tree, QuadtreeNode *node)
{
	OpenGLLodMap *lodmap = g_hash_table_lookup(maps, tree);
	assert(lodmap != NULL);

	OpenGLLodMapTile *tile = ALLOCATE_OBJECT(OpenGLLodMapTile);

	if(node->level == 0) { // load the lowest level from disk
		GString *path = g_string_new(lodmap->dataPrefix);
		g_string_append_printf(path, ".%d.%d.%s", node->x, node->y, lodmap->dataSuffix);

		LOG_DEBUG("Loading LOD map image tile (%d,%d) from %s", node->x, node->y, path->str);
		tile->heights = $(Image *, image, readImageFromFile)(path->str);
		g_string_free(path, true);

		if(tile->heights != NULL && (tile->heights->width != tree->leafSize || tile->heights->height != tree->leafSize)) { // invalid map, we need to recover
			$(void, image, freeImage)(tile->heights);
			tile->heights = NULL;
		}

		if(tile->heights == NULL) { // failed to load, we need to recover
			tile->heights = $(Image *, image, createImageFloat)(tree->leafSize, tree->leafSize, 1);
			$(void, image, clearImage)(tile->heights);
		}
	} else { // construct higher LOD levels by downsampling the lower detailed ones
		// these should be preloaded
		assert(quadtreeNodeDataIsLoaded(node->children[0]));
		assert(quadtreeNodeDataIsLoaded(node->children[1]));
		assert(quadtreeNodeDataIsLoaded(node->children[2]));
		assert(quadtreeNodeDataIsLoaded(node->children[3]));

		LOG_DEBUG("Upsampling LOD map image tile (%hd,%hd) for level %hd", node->x, node->y, node->level);

		// create the image to which we upsample
		tile->heights = $(Image *, image, createImageFloat)(tree->leafSize, tree->leafSize, 1);

		// upsample the heights
		unsigned int halfsize = tree->leafSize / 2;
		for(unsigned int y = 0; y < tree->leafSize; y++) {
			bool isLowerY = y < halfsize;
			int offsetY = isLowerY ? 0 : -tree->leafSize;

			for(unsigned int x = 0; x < tree->leafSize; x++) {
				bool isLowerX = x < halfsize;
				int offsetX = isLowerX ? 0 : -tree->leafSize;

				// determine correct sub image
				unsigned int index = (isLowerX ? 0 : 1) + (isLowerY ? 0 : 2);
				Image *subheights = ((OpenGLLodMapTile *) node->children[index]->data)->heights;

				// upsample the height
				float value = 0.25f * (getImage(subheights, offsetX + 2 * x, offsetY + 2 * y, 0) + getImage(subheights, offsetX + 2 * x + 1, offsetY + 2 * y, 0) + getImage(subheights, offsetX + 2 * x, offsetY + 2 * y + 1, 0) + getImage(subheights, offsetX + 2 * x + 1, offsetY + 2 * y + 1, 0));
				setImage(tile->heights, x, y, 0, value);
			}
		}
	}

	// Compute normals
	tile->normals = $(Image *, image, createImageFloat)(tree->leafSize, tree->leafSize, 3);
	$(void, heightmap, computeHeightmapNormals)(tile->heights, tile->normals);

	return tile;
}

/**
 * Frees an LOD map tile
 *
 * @param tree			the quadtree for which to free the map tile
 * @param data			the LOD map tile to free
 */
static void freeLodMapTile(Quadtree *tree, void *data)
{
	OpenGLLodMapTile *tile = data;
	free(tile);
}
