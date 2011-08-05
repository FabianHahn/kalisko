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

#include <limits.h>
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
#include "intersect.h"

// #define LODMAP_VERBOSE

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 6, 7);
MODULE_BCVERSION(0, 6, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("heightmap", 0, 4, 0), MODULE_DEPENDENCY("quadtree", 0, 8, 0), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 1, 4), MODULE_DEPENDENCY("linalg", 0, 3, 4));

static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node, double time);
static void *loadLodMapTile(Quadtree *tree, QuadtreeNode *node);
static OpenGLHeightmapDrawMode getDrawModeForIndex(int index);
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
 * @param camera				the OpenGL camera with respect to which the OpenGL map should be updated
 * @param baseRange				the base viewing range in world coordinates covered by the lowest LOD level in the LOD map
 * @param viewingDistance		the maximum viewing distance in LDO levels to be handled by this LOD map
 * @param leafSize				the leaf size of the LOD map
 * @param dataPrefix			the prefix that should be prepended to all loaded tiles
 * @param dataSuffix			the suffix that should be appended to all loaded tiles
 * @result						the created OpenGL LOD map
 */
API OpenGLLodMap *createOpenGLLodMap(double baseRange, unsigned int viewingDistance, unsigned int leafSize, const char *dataPrefix, const char *dataSuffix)
{
	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->heightmap = $(OpenGLPrimitive *, heightmap, createOpenGLPrimitiveHeightmap)(NULL, leafSize + 1, leafSize + 1); // create a managed heightmap that will serve as instance for our rendered tiles
	lodmap->quadtree = $(Quadtree *, quadtree, createQuadtree)(leafSize, 512, &loadLodMapTile, &freeLodMapTile, true);
	lodmap->selection = NULL;
	lodmap->viewerPosition = $(Vector *, linalg, createVector3)(0.0, 0.0, 0.0);
	lodmap->baseRange = baseRange;
	lodmap->viewingDistance = viewingDistance;
	lodmap->dataPrefix = strdup(dataPrefix);
	lodmap->dataSuffix = strdup(dataSuffix);
	lodmap->polygonMode = GL_FILL;
	lodmap->morphStartFactor = 0.8f;

	// create lodmap material
	$(bool, opengl, deleteOpenGLMaterial)("lodmap");
	char *execpath = $$(char *, getExecutablePath)();
	GString *vertexShaderPath = g_string_new(execpath);
	g_string_append_printf(vertexShaderPath, "/modules/lodmap/lodmap.glslv");
	GString *fragmentShaderPath = g_string_new(execpath);
	g_string_append_printf(fragmentShaderPath, "/modules/lodmap/lodmap.glslf");

	bool result = $(bool, opengl, createOpenGLMaterialFromFiles)("lodmap", vertexShaderPath->str, fragmentShaderPath->str);

	g_string_free(vertexShaderPath, true);
	g_string_free(fragmentShaderPath, true);
	free(execpath);

	if(!result) {
		$(void, heightmap, freeOpenGLPrimitiveHeightmap)(lodmap->heightmap);
		$(void, quadtree, freeQuadtree)(lodmap->quadtree);
		free(lodmap->dataPrefix);
		free(lodmap->dataSuffix);
		LOG_ERROR("Failed to create OpenGL LOD map material");
		return NULL;
	}

	// add lodmap configuration uniforms
	OpenGLUniformAttachment *uniforms = $(OpenGLUniformAttachment *, opengl, getOpenGLMaterialUniforms)("lodmap");
	$(bool, opengl, attachOpenGLUniform)(uniforms, "baseRange", $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&lodmap->baseRange));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "morphStartFactor", $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&lodmap->morphStartFactor));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "viewerPosition", $(OpenGLUniform *, opengl, createOpenGLUniformVector)(lodmap->viewerPosition));

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
 * @param position		the viewer position for which the LOD map should be updated
 * @param autoExpand	specifies whether the quadtree should be automatically expanded to ranged not covered yet
 */
API void updateOpenGLLodMap(OpenGLLodMap *lodmap, Vector *position, bool autoExpand)
{
	// update the viewer position
	$(void, linalg, assignVector)(lodmap->viewerPosition, position);

	// prune the quadtree to get rid of unused nodes
	$(void, quadtree, pruneQuadtree)(lodmap->quadtree);

	if(autoExpand) {
		// expand the quadtree to actually cover our position
		float *positionData = $(float *, linalg, getVectorData)(position);
		$(void, quadtree, expandQuadtreeWorld)(lodmap->quadtree, positionData[0], positionData[2]);
	}

	double range = getLodMapNodeRange(lodmap, lodmap->quadtree->root);
	QuadtreeAABB box = quadtreeNodeAABB(lodmap->quadtree, lodmap->quadtree->root);
	LOG_DEBUG("Updating LOD map for quadtree covering range [%d,%d]x[%d,%d] on %u levels", box.minX, box.maxX, box.minY, box.maxY, lodmap->quadtree->root->level);

	// free our previous selection list
	g_list_free(lodmap->selection);
	lodmap->selection = NULL;

	// select the LOD map nodes to be rendered
	if(lodmapQuadtreeNodeIntersectsSphere(lodmap->quadtree->root, position, range)) {
		double time = $$(double, getMicroTime)();
		lodmap->selection = selectLodMapNodes(lodmap, position, lodmap->quadtree->root, time);

		// make all selected nodes visible
		for(GList *iter = lodmap->selection; iter != NULL; iter = iter->next) {
			QuadtreeNode *node = iter->data;
			assert(quadtreeNodeDataIsLoaded(node));
			OpenGLLodMapTile *tile = node->data;
			tile->model->visible = true; // make model visible for rendering
			tile->model->polygonMode = lodmap->polygonMode; // set the parent's polygon mode
		}

		LOG_DEBUG("Selected %u LOD map nodes", g_list_length(lodmap->selection));
	}
}

/**
 * Draws an OpenGL LOD map
 *
 * @param lodmap		the LOD map to draw
 */
API void drawOpenGLLodMap(OpenGLLodMap *lodmap)
{
	for(GList *iter = lodmap->selection; iter != NULL; iter = iter->next) {
		QuadtreeNode *node = iter->data;
		assert(quadtreeNodeDataIsLoaded(node));
		OpenGLLodMapTile *tile = node->data;
		$(bool, opengl, drawOpenGLModel)(tile->model, &tile->drawOptions);
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

	$(bool, opengl, deleteOpenGLMaterial)("lodmap");
	freeOpenGLPrimitive(lodmap->heightmap);
	$(void, linalg, freeVector)(lodmap->viewerPosition);
	free(lodmap->dataPrefix);
	free(lodmap->dataSuffix);
	free(lodmap);
}

/**
 * Recursively selects nodes from an LOD map's quadtree for a LOD query
 *
 * @param lodmap		the LOD map for which to select quadtree nodes
 * @param position		the viewer position with respect to which the LOD map should be updated
 * @param node			the current quadtree node we're traversing
 * @param time			the current timestamp to set for caching
 */
static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node, double time)
{
	GList *nodes = NULL;

	node->time = time; // update access time

	double range = getLodMapNodeRange(lodmap, node);
	if(!lodmapQuadtreeNodeIntersectsSphere(node, position, range) && node->level > lodmap->viewingDistance) { // the node is outside its LOD viewing range and beyond the viewing distance, so cut it
		return NULL;
	}

	bool drawn = true;
	OpenGLHeightmapDrawOptions options;
	options.drawMode = OPENGL_HEIGHTMAP_DRAW_ALL;

	if(!quadtreeNodeIsLeaf(node)) {
		// we need to check which of our children are in their viewing range as well
		for(unsigned int i = 0; i < 4; i++) {
			QuadtreeNode *child = node->children[i];
			OpenGLHeightmapDrawMode drawMode = getDrawModeForIndex(i);
			double subrange = getLodMapNodeRange(lodmap, child);
			if(lodmapQuadtreeNodeIntersectsSphere(child, position, subrange)) { // the child node is insite its LOD viewing range for the current viewer position
				options.drawMode ^= drawMode; // don't draw this part if the child covers it

				GList *childNodes = selectLodMapNodes(lodmap, position, child, time); // node selection
				nodes = g_list_concat(nodes, childNodes); // append the child's nodes to our selection
			}
		}

		if(options.drawMode == OPENGL_HEIGHTMAP_DRAW_NONE) { // if all are, we're not drawn
			drawn = false;
		}

		// update node weight (we could have loaded child nodes)
		node->weight = node->children[0]->weight + node->children[1]->weight + node->children[2]->weight + node->children[3]->weight + (quadtreeNodeDataIsLoaded(node) ? 1 : 0);
	}

	if(drawn) {
#ifdef LODMAP_VERBOSE
		QuadtreeAABB box = quadtreeNodeAABB(lodmap->quadtree, node);
		LOG_DEBUG("Selected node range [%d,%d]x[%d,%d] at level %u", box.minX, box.maxX, box.minY, box.maxY, node->level);
#endif

		// Now make sure the node data is loaded
		if(!quadtreeNodeDataIsLoaded(node)) {
			$(void *, quadtree, loadQuadtreeNodeData)(lodmap->quadtree, node); // load this node's data - our recursion parents will make sure they update their nodes' weights
		}

		OpenGLLodMapTile *tile = node->data;
		tile->drawOptions = options;

		nodes = g_list_append(nodes, node); // select ourselves
	}

	return nodes;
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
		Image *heightmaps[9];

		// Load all adjacent tiles so we can interpolate
		for(int i = 0; i < 3; i++) {
			int tileY = node->y - 1 + i;
			for(int j = 0; j < 3; j++) {
				int tileX = node->x - 1 + j;
				int hi = 3 * i + j;

				GString *path = g_string_new(lodmap->dataPrefix);
				g_string_append_printf(path, ".%d.%d.%s", tileX, tileY, lodmap->dataSuffix);

#ifdef LODMAP_VERBOSE
				LOG_DEBUG("Loading LOD map image tile (%d,%d) from %s as heightmap %d for interpolation", tileX, tileY, path->str, hi);
#endif
				heightmaps[hi] = $(Image *, image, readImageFromFile)(path->str);
				g_string_free(path, true);

				if(heightmaps[hi] != NULL && (heightmaps[hi]->width != tree->leafSize || heightmaps[hi]->height != tree->leafSize)) { // invalid map, we need to recover
					$(void, image, freeImage)(heightmaps[hi]);
					heightmaps[hi] = NULL;
				}

				if(heightmaps[hi] == NULL) { // failed to load, we need to recover
#ifdef LODMAP_VERBOSE
					LOG_DEBUG("Failed to load heightmap %d for LOD map interpolation, setting to zero", hi);
#endif
					heightmaps[hi] = $(Image *, image, createImageFloat)(tree->leafSize, tree->leafSize, 1);
					$(void, image, clearImage)(heightmaps[hi]);
				}
			}
		}

		LOG_DEBUG("Interpolating LOD map base tile (%d,%d)", node->x, node->y);
		tile->heights = $(Image *, image, createImageFloat)(tree->leafSize + 1, tree->leafSize + 1, 1);
		tile->minHeight = FLT_MAX;
		tile->maxHeight = FLT_MIN;

		// inner part
		for(unsigned int y = 1; y < tree->leafSize; y++) {
			for(unsigned int x = 1; x < tree->leafSize; x++) {
				// interpolate the height
				float value = 0.25f * (getImage(heightmaps[4], x - 1, y - 1, 0) + getImage(heightmaps[4], x - 1, y, 0) + getImage(heightmaps[4], x, y - 1, 0) + getImage(heightmaps[4], x, y, 0));
				setImage(tile->heights, x, y, 0, value);

				// update min/max
				if(value < tile->minHeight) {
					tile->minHeight = value;
				}

				if(value > tile->maxHeight) {
					tile->maxHeight = value;
				}
			}
		}

		// bottom border with heightmap 7
		for(unsigned int x = 1; x < tree->leafSize; x++) {
			// interpolate the height
			float value = 0.25f * (getImage(heightmaps[7], x - 1, 0, 0) + getImage(heightmaps[7], x, 0, 0) + getImage(heightmaps[4], x - 1, tree->leafSize - 1, 0) + getImage(heightmaps[4], x, tree->leafSize - 1, 0));
			setImage(tile->heights, x, tree->leafSize, 0, value);
		}

		// top border with heightmap 1
		for(unsigned int x = 1; x < tree->leafSize; x++) {
			// interpolate the height
			float value = 0.25f * (getImage(heightmaps[4], x - 1, 0, 0) + getImage(heightmaps[4], x, 0, 0) + getImage(heightmaps[1], x - 1, tree->leafSize - 1, 0) + getImage(heightmaps[1], x, tree->leafSize - 1, 0));
			setImage(tile->heights, x, 0, 0, value);
		}

		// left border with heightmap 3
		for(unsigned int y = 1; y < tree->leafSize; y++) {
			// interpolate the height
			float value = 0.25f * (getImage(heightmaps[4], 0, y, 0) + getImage(heightmaps[4], 0, y - 1, 0) + getImage(heightmaps[3], tree->leafSize - 1, y, 0) + getImage(heightmaps[3], tree->leafSize - 1, y - 1, 0));
			setImage(tile->heights, 0, y, 0, value);
		}

		// right border with heightmap 5
		for(unsigned int y = 1; y < tree->leafSize; y++) {
			// interpolate the height
			float value = 0.25f * (getImage(heightmaps[5], 0, y, 0) + getImage(heightmaps[5], 0, y - 1, 0) + getImage(heightmaps[4], tree->leafSize - 1, y, 0) + getImage(heightmaps[4], tree->leafSize - 1, y - 1, 0));
			setImage(tile->heights, tree->leafSize, y, 0, value);
		}

		// bottom left corner with heightmap 6
		setImage(tile->heights, 0, tree->leafSize, 0, 0.25f * (getImage(heightmaps[7], 0, 0, 0) + getImage(heightmaps[6], tree->leafSize - 1, 0, 0) + getImage(heightmaps[3], tree->leafSize - 1, tree->leafSize - 1, 0) + getImage(heightmaps[4], 0, tree->leafSize - 1, 0)));

		// bottom right corner with heightmap 8
		setImage(tile->heights, tree->leafSize, tree->leafSize, 0, 0.25f * (getImage(heightmaps[8], 0, 0, 0) + getImage(heightmaps[7], tree->leafSize - 1, 0, 0) + getImage(heightmaps[4], tree->leafSize - 1, tree->leafSize - 1, 0) + getImage(heightmaps[5], 0, tree->leafSize - 1, 0)));

		// top left corner with heightmap 0
		setImage(tile->heights, 0, 0, 0, 0.25f * (getImage(heightmaps[4], 0, 0, 0) + getImage(heightmaps[3], tree->leafSize - 1, 0, 0) + getImage(heightmaps[0], tree->leafSize - 1, tree->leafSize - 1, 0) + getImage(heightmaps[1], 0, tree->leafSize - 1, 0)));

		// top right corner with heightmap 2
		setImage(tile->heights, tree->leafSize, 0, 0, 0.25f * (getImage(heightmaps[5], 0, 0, 0) + getImage(heightmaps[4], tree->leafSize - 1, 0, 0) + getImage(heightmaps[1], tree->leafSize - 1, tree->leafSize - 1, 0) + getImage(heightmaps[2], 0, tree->leafSize - 1, 0)));

		// free the loaded heightmaps
		for(int k = 0; k < 9; k++) {
			$(void, image, freeImage)(heightmaps[k]);
		}
	} else { // construct higher LOD levels by downsampling the lower detailed ones
		// these should be preloaded
		assert(quadtreeNodeDataIsLoaded(node->children[0]));
		assert(quadtreeNodeDataIsLoaded(node->children[1]));
		assert(quadtreeNodeDataIsLoaded(node->children[2]));
		assert(quadtreeNodeDataIsLoaded(node->children[3]));

		LOG_DEBUG("Propagating LOD map image tile (%hd,%hd) for level %hd", node->x, node->y, node->level);

		// create the image to which we propagate
		tile->heights = $(Image *, image, createImageFloat)(tree->leafSize + 1, tree->leafSize + 1, 1);

		// propagate the heights
		for(unsigned int y = 0; y < (tree->leafSize + 1); y++) {
			unsigned int dy = 2 * y;
			bool isLowerY = (2 * y) < (tree->leafSize + 1);
			int offsetY = isLowerY ? 0 : -tree->leafSize;

			for(unsigned int x = 0; x < (tree->leafSize + 1); x++) {
				unsigned int dx = 2 * x;
				bool isLowerX = dx < (tree->leafSize + 1);
				int offsetX = isLowerX ? 0 : -tree->leafSize;

				// determine correct sub image
				unsigned int index = (isLowerX ? 0 : 1) + (isLowerY ? 0 : 2);
				Image *subheights = ((OpenGLLodMapTile *) node->children[index]->data)->heights;

				// propagate the height
				setImage(tile->heights, x, y, 0, getImage(subheights, dx + offsetX, dy + offsetY, 0));
			}
		}

		// set min/max
		tile->minHeight = FLT_MAX;
		tile->maxHeight = FLT_MIN;

		for(unsigned int i = 0; i < 4; i++) {
			OpenGLLodMapTile *subtile = node->children[i]->data;

			if(subtile->minHeight < tile->minHeight) {
				tile->minHeight = subtile->minHeight;
			}

			if(subtile->maxHeight > tile->maxHeight) {
				tile->maxHeight = subtile->maxHeight;
			}
		}
	}

	// Compute normals
	tile->normals = $(Image *, image, createImageFloat)(tree->leafSize + 1, tree->leafSize + 1, 3);
	$(void, heightmap, computeHeightmapNormals)(tile->heights, tile->normals);

	// Create OpenGL textures
	tile->heightsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(tile->heights);
	tile->normalsTexture = $(OpenGLTexture *, opengl, createOpenGLVertexTexture2D)(tile->normals);

	// Create OpenGL model
	tile->model = $(OpenGLModel *, opengl, createOpenGLModel)(lodmap->heightmap);
	$(bool, opengl, attachOpenGLModelMaterial)(tile->model, "lodmap");

	// Set model transform
	unsigned int scale = quadtreeNodeScale(node);
	float *positionData = $(float *, linalg, getVectorData)(tile->model->translation);
	positionData[0] = node->x;
	positionData[1] = 0;
	positionData[2] = node->y; // y in model coordinates is z in world coordinates
	tile->model->scaleX = scale;
	tile->model->scaleY = 1.0f;
	tile->model->scaleZ = scale;
	$(void, opengl, updateOpenGLModelTransform)(tile->model);

	// Attach textures to model
	OpenGLUniformAttachment *uniforms = tile->model->uniforms;
	OpenGLUniform *heightsTextureUniform = $(OpenGLUniform *, opengl, getOpenGLUniform)(uniforms, "heights");
	assert(heightsTextureUniform != NULL);
	heightsTextureUniform->content.texture_value = tile->heightsTexture;
	OpenGLUniform *normalsTextureUniform = $(OpenGLUniform *, opengl, getOpenGLUniform)(uniforms, "normals");
	assert(normalsTextureUniform != NULL);
	normalsTextureUniform->content.texture_value = tile->normalsTexture;
	$(bool, opengl, attachOpenGLUniform)(uniforms, "lodLevel", $(OpenGLUniform *, opengl, createOpenGLUniformInt)(node->level));

	// Make model invisible
	tile->model->visible = false;

	return tile;
}

/**
 * Returns the OpenGL heightmap draw mode for a given child index
 *
 * @param index			the child index to look the draw mode up for
 * @result				the drawing mode
 */
static OpenGLHeightmapDrawMode getDrawModeForIndex(int index)
{
	switch(index) {
		case 0:
			return OPENGL_HEIGHTMAP_DRAW_TOP_LEFT;
		break;
		case 1:
			return OPENGL_HEIGHTMAP_DRAW_TOP_RIGHT;
		break;
		case 2:
			return OPENGL_HEIGHTMAP_DRAW_BOTTOM_LEFT;
		break;
		case 3:
			return OPENGL_HEIGHTMAP_DRAW_BOTTOM_RIGHT;
		break;
		default:
			return OPENGL_HEIGHTMAP_DRAW_NONE;
		break;
	}
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
	$(void, opengl, freeOpenGLModel)(tile->model);
	$(void, opengl, freeOpenGLTexture)(tile->heightsTexture);
	$(void, opengl, freeOpenGLTexture)(tile->normalsTexture);
	free(tile);
}
