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
#include "api.h"
#include "lodmap.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 1, 6);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 27, 0), MODULE_DEPENDENCY("heightmap", 0, 2, 13), MODULE_DEPENDENCY("quadtree", 0, 7, 0), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 1, 4));

static void *loadLodMapTile(Quadtree *tree, QuadtreeNode *node);
static void freeLodMapTile(Quadtree *tree, void *data);

/**
 * A hash table associating quadtree objects with their parent LOD map primitives
 */
static GHashTable *maps;

MODULE_INIT
{
	maps = g_hash_table_new(&g_direct_hash, &g_direct_equal);

#if 1 // test code
	$$(void, breakpoint)();

	OpenGLPrimitive *primitive = createOpenGLPrimitiveLodMap(128, "/home/smf68/kaliskomap/map", "png");
	OpenGLLodMap *lodmap = primitive->data;
	$(QuadtreeNode *, quadtree, lookupQuadtreeNode)(lodmap->quadtree, 3 * 128, 3 * 128, 0);
	OpenGLLodMapTile *tile = $(void *, quadtree, lookupQuadtree)(lodmap->quadtree, 0.0, 0.0, 2);
	$(void, image, debugImage)(tile->heights);
	$(void, image, debugImage)(tile->normals);

	freeOpenGLPrimitiveLodMap(primitive);
#endif

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(maps);
}

/**
 * Creates an OpenGL LOD map primitive
 *
 * @result			the created OpenGL LOD map primitve
 */
API OpenGLPrimitive *createOpenGLPrimitiveLodMap(unsigned int leafSize, const char *dataPrefix, const char *dataSuffix)
{
	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->quadtree = $(Quadtree *, quadtree, createQuadtree)(leafSize, 25, &loadLodMapTile, &freeLodMapTile, true);
	lodmap->dataPrefix = strdup(dataPrefix);
	lodmap->dataSuffix = strdup(dataSuffix);
	lodmap->primitive.type = "lodmap";
	lodmap->primitive.data = lodmap;
	lodmap->primitive.setup_function = NULL;
	lodmap->primitive.draw_function = NULL;
	lodmap->primitive.update_function = NULL;
	lodmap->primitive.free_function = &freeOpenGLPrimitiveLodMap;

	g_hash_table_insert(maps, lodmap->quadtree, lodmap);

	return &lodmap->primitive;
}

/**
 * Frees an OpenGL LOD map primitive
 *
 * @param primitive				the OpenGL LOD map primitive to free
 */
API void freeOpenGLPrimitiveLodMap(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "lodmap") != 0) {
		LOG_ERROR("Failed to free OpenGL LOD map: Primitive is not a LOD map");
		return;
	}

	OpenGLLodMap *lodmap = primitive->data;

	g_hash_table_remove(maps, lodmap->quadtree);
	$(void, quadtree, freeQuadtree)(lodmap->quadtree);
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
		int tileX = node->x / tree->leafSize;
		int tileY = node->y / tree->leafSize;

		GString *path = g_string_new(lodmap->dataPrefix);
		g_string_append_printf(path, ".%d.%d.%s", tileX, tileY, lodmap->dataSuffix);

		LOG_DEBUG("Loading LOD map image tile (%d,%d) from %s", tileX, tileY, path->str);
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
