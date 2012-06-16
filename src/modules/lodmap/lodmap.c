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
#include "modules/heightmap/heightmap.h"
#include "modules/heightmap/normals.h"
#include "modules/opengl/material.h"
#include "modules/linalg/Vector.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/clone.h"
#define API
#include "lodmap.h"
#include "intersect.h"
#include "source.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 18, 1);
MODULE_BCVERSION(0, 14, 3);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 12), MODULE_DEPENDENCY("heightmap", 0, 4, 4), MODULE_DEPENDENCY("quadtree", 0, 12, 2), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 2, 0), MODULE_DEPENDENCY("linalg", 0, 3, 4), MODULE_DEPENDENCY("store", 0, 6, 12));

static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node);
static void preloadLodMapNode(OpenGLLodMap *lodmap, QuadtreeNode *node);
static void createLodMapTile(Quadtree *tree, QuadtreeNode *node);
static void activateLodMapTile(OpenGLLodMap *lodmap, QuadtreeNode *node);
static void deactivateLodMapTile(OpenGLLodMapTile *tile);
static void unloadLodMapTile(OpenGLLodMapTile *tile);
static OpenGLHeightmapDrawMode getDrawModeForIndex(int index);
static void freeLodMapTile(Quadtree *tree, void *data);

/**
 * A hash table associating quadtree objects with their parent LOD map primitives
 */
static GHashTable *maps;

MODULE_INIT
{
	maps = g_hash_table_new(&g_direct_hash, &g_direct_equal);
	initOpenGLLodMapDataSourceFactories();

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(maps);
	freeOpenGLLodMapDataSourceFactories();
}

/**
 * Creates an OpenGL LOD map from a store representation
 *
 * @param store			the store config from which to create the LOD map
 * @result				the created LOD map or NULL on failure
 */
API OpenGLLodMap *createOpenGLLodMapFromStore(Store *storeConfig)
{
	Store *store = cloneStore(storeConfig);

	OpenGLLodMapDataSource *source = createOpenGLLodMapDataSourceFromStore(store);
	if(source == NULL) {
		LOG_ERROR("Failed to create LOD map from store: Failed to create LOD map data source!");
		freeStore(store);
		return NULL;
	}

	Store *paramBaseRange = getStorePath(store, "lodmap/baseRange");
	if(paramBaseRange == NULL || !(paramBaseRange->type == STORE_INTEGER || paramBaseRange->type == STORE_FLOAT_NUMBER)) {
		LOG_ERROR("Failed to create LOD map from store: Required config float value 'lodmap/baseRange' not found!");
		freeStore(store);
		return NULL;
	}

	double baseRange = paramBaseRange->type == STORE_FLOAT_NUMBER ? paramBaseRange->content.float_number : paramBaseRange->content.integer;

	Store *paramViewingDistance = getStorePath(store, "lodmap/viewingDistance");
	if(paramViewingDistance == NULL || paramViewingDistance->type != STORE_INTEGER) {
		LOG_ERROR("Failed to create LOD map from store: Required config integer value 'lodmap/viewingDistance' not found!");
		freeStore(store);
		return NULL;
	}

	int viewingDistance = paramViewingDistance->content.integer;

	OpenGLLodMap *lodmap = createOpenGLLodMap(source, baseRange, viewingDistance);
	if(lodmap == NULL) {
		freeOpenGLLodMapDataSource(source);
		freeStore(store);
		return NULL;
	}

	if(getStorePath(store, "lodmap/quadtree") != NULL) {
		Store *paramRootX;
		int rootX;
		if((paramRootX = getStorePath(store, "lodmap/quadtree/rootX")) == NULL || paramRootX->type != STORE_INTEGER) {
			LOG_WARNING("LOD map config integer value 'lodmap/quadtree/rootX' not found, using default value of '0'");
			rootX = 0;
		} else {
			rootX = paramRootX->content.integer;
		}

		Store *paramRootY;
		int rootY;
		if((paramRootY = getStorePath(store, "lodmap/quadtree/rootY")) == NULL || paramRootY->type != STORE_INTEGER) {
			LOG_WARNING("LOD map config integer value 'lodmap/quadtree/rootY' not found, using default value of '0'");
			rootY = 0;
		} else {
			rootY = paramRootY->content.integer;
		}

		Store *paramRootLevel;
		int rootLevel;
		if((paramRootLevel = getStorePath(store, "lodmap/quadtree/rootLevel")) == NULL || paramRootLevel->type != STORE_INTEGER) {
			LOG_WARNING("LOD map config integer value 'lodmap/quadtree/rootLevel' not found, using default value of '0'");
			rootLevel = 0;
		} else {
			rootLevel = paramRootLevel->content.integer;
		}

		reshapeQuadtree(lodmap->quadtree, rootX, rootY, rootLevel);
	}

	freeStore(store);
	return lodmap;
}

/**
 * Creates an OpenGL LOD map
 *
 * @param source				the data source used for the LOD map (note that the LOD map takes over control over this data source, i.e. you must not free it once this function succeeded)
 * @param baseRange				the base viewing range in world coordinates covered by the lowest LOD level in the LOD map
 * @param viewingDistance		the maximum viewing distance in LDO levels to be handled by this LOD map
 * @result						the created OpenGL LOD map
 */
API OpenGLLodMap *createOpenGLLodMap(OpenGLLodMapDataSource *source, double baseRange, unsigned int viewingDistance)
{
	unsigned int tileSize = getLodMapImageSize(source, OPENGL_LODMAP_IMAGE_HEIGHT);

	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->source = source;
	lodmap->heightmap = createOpenGLPrimitiveHeightmap(NULL, tileSize, tileSize); // create a managed heightmap that will serve as instance for our rendered tiles
	lodmap->quadtree = createQuadtree(&createLodMapTile, &freeLodMapTile);
	lodmap->selection = NULL;
	lodmap->viewerPosition = createVector3(0.0, 0.0, 0.0);
	lodmap->baseRange = baseRange;
	lodmap->viewingDistance = viewingDistance;
	lodmap->polygonMode = GL_FILL;
	lodmap->morphStartFactor = 0.8f;
	lodmap->loadingPool = g_thread_pool_new(&loadLodMapTile, lodmap, -1, false, NULL);

	// create lodmap material
	deleteOpenGLMaterial("lodmap");
	char *execpath = getExecutablePath();
	GString *vertexShaderPath = g_string_new(execpath);
	g_string_append_printf(vertexShaderPath, "/modules/lodmap/lodmap.glslv");
	GString *fragmentShaderPath = g_string_new(execpath);
	g_string_append_printf(fragmentShaderPath, "/modules/lodmap/lodmap.glslf");

	bool result = createOpenGLMaterialFromFiles("lodmap", vertexShaderPath->str, fragmentShaderPath->str);

	g_string_free(vertexShaderPath, true);
	g_string_free(fragmentShaderPath, true);
	free(execpath);

	if(!result) {
		freeOpenGLPrimitiveHeightmap(lodmap->heightmap);
		freeQuadtree(lodmap->quadtree);
		LOG_ERROR("Failed to create OpenGL LOD map material");
		return NULL;
	}

	// add lodmap configuration uniforms
	OpenGLUniformAttachment *uniforms = getOpenGLMaterialUniforms("lodmap");
	attachOpenGLUniform(uniforms, "baseRange", createOpenGLUniformFloatPointer(&lodmap->baseRange));
	attachOpenGLUniform(uniforms, "morphStartFactor", createOpenGLUniformFloatPointer(&lodmap->morphStartFactor));
	attachOpenGLUniform(uniforms, "viewerPosition", createOpenGLUniformVector(lodmap->viewerPosition));

	g_hash_table_insert(maps, lodmap->quadtree, lodmap);

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
	assignVector(lodmap->viewerPosition, position);

	if(autoExpand) {
		// expand the quadtree to actually cover our position
		float *positionData = getVectorData(position);
		expandQuadtree(lodmap->quadtree, positionData[0], positionData[2]);
	}

	double range = getLodMapNodeRange(lodmap, lodmap->quadtree->root);

	// free our previous selection list
	g_list_free(lodmap->selection);
	lodmap->selection = NULL;

	// select the LOD map nodes to be rendered
	if(lodmapQuadtreeNodeIntersectsSphere(lodmap, lodmap->quadtree->root, position, range)) {
		lodmap->selection = selectLodMapNodes(lodmap, position, lodmap->quadtree->root);

		// make all selected nodes visible
		for(GList *iter = lodmap->selection; iter != NULL; iter = iter->next) {
			QuadtreeNode *node = iter->data;
			OpenGLLodMapTile *tile = node->data;

			// wait until the node is fully loaded
			g_mutex_lock(tile->mutex);
			while(tile->status == OPENGL_LODMAP_TILE_LOADING) {
				g_cond_wait(tile->condition, tile->mutex);
			}
			g_mutex_unlock(tile->mutex);

			// activate if not active yet
			if(tile->status != OPENGL_LODMAP_TILE_ACTIVE) {
				activateLodMapTile(lodmap, node);
			}

			tile->model->visible = true; // make model visible for rendering
			tile->model->polygonMode = lodmap->polygonMode; // set the parent's polygon mode
		}

		QuadtreeAABB box = quadtreeNodeAABB(lodmap->quadtree->root);
		LOG_DEBUG("Updated LOD map for quadtree covering range [%d,%d]x[%d,%d] on %u levels: %d nodes selected", box.minX, box.maxX, box.minY, box.maxY, lodmap->quadtree->root->level, g_list_length(lodmap->selection));
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
		OpenGLLodMapTile *tile = node->data;
		drawOpenGLModel(tile->model, &tile->drawOptions);
	}
}

/**
 * Loads an LOD map tile.
 *
 * @param node_p		a pointer to the quadtree node for which to load the tile
 * @param lodmap_p		a pointer to the LOD map for which to load the tile*
 */
API void loadLodMapTile(void *node_p, void *lodmap_p)
{
	QuadtreeNode *node = node_p;
	OpenGLLodMap *lodmap = lodmap_p;
	OpenGLLodMapTile *tile = node->data;

	assert(tile->status == OPENGL_LODMAP_TILE_LOADING);
	g_mutex_lock(tile->mutex); // lock the node so we can properly edit it

	tile->heights = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_HEIGHT, node->x, node->y, node->level, &tile->minHeight, &tile->maxHeight);
	tile->normals = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_NORMALS, node->x, node->y, node->level, NULL, NULL);
	tile->texture = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_TEXTURE, node->x, node->y, node->level, NULL, NULL);

	// apply correct scaling to the height limits date which will be used for 3D distance checks
	tile->minHeight *= lodmap->source->heightRatio;
	tile->maxHeight *= lodmap->source->heightRatio;

	tile->status = OPENGL_LODMAP_TILE_READY;

	// notify waiting threads and release the lock
	g_cond_signal(tile->condition);
	g_mutex_unlock(tile->mutex);
}

/**
 * Frees an OpenGL LOD map
 *
 * @param lodmap				the OpenGL LOD map to free
 */
API void freeOpenGLLodMap(OpenGLLodMap *lodmap)
{
	g_hash_table_remove(maps, lodmap->quadtree);

	g_thread_pool_free(lodmap->loadingPool, false, true);
	freeQuadtree(lodmap->quadtree);

	deleteOpenGLMaterial("lodmap");
	freeOpenGLPrimitive(lodmap->heightmap);
	freeVector(lodmap->viewerPosition);
	freeOpenGLLodMapDataSource(lodmap->source);

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
static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node)
{
	GList *nodes = NULL;

	double range = getLodMapNodeRange(lodmap, node);
	if(!lodmapQuadtreeNodeIntersectsSphere(lodmap, node, position, range) && node->level > lodmap->viewingDistance) { // the node is outside its LOD viewing range and beyond the viewing distance, so cut it
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
			if(lodmapQuadtreeNodeIntersectsSphere(lodmap, child, position, subrange)) { // the child node is insite its LOD viewing range for the current viewer position
				options.drawMode ^= drawMode; // don't draw this part if the child covers it

				OpenGLLodMapTile *childTile = child->data;
				if(childTile->status == OPENGL_LODMAP_TILE_META) { // we need to load this node
					childTile->status = OPENGL_LODMAP_TILE_LOADING;
					g_thread_pool_push(lodmap->loadingPool, child, NULL);
				}

				GList *childNodes = selectLodMapNodes(lodmap, position, child); // node selection
				nodes = g_list_concat(nodes, childNodes); // append the child's nodes to our selection
			} else { // not in viewing range yet, but might be soon, so preload it
				preloadLodMapNode(lodmap, child);
			}
		}

		if(options.drawMode == OPENGL_HEIGHTMAP_DRAW_NONE) { // if all are, we're not drawn
			drawn = false;
		}
	}

	if(drawn) {
		OpenGLLodMapTile *tile = node->data;
		tile->drawOptions = options;

		if(tile->status == OPENGL_LODMAP_TILE_META) { // we need to load this node
			tile->status = OPENGL_LODMAP_TILE_LOADING;
			g_thread_pool_push(lodmap->loadingPool, tile, NULL);
		}

		nodes = g_list_append(nodes, node); // select ourselves
	}

	return nodes;
}


/**
 * Preloads a given LOD map node and unloads its children if they were previously preloaded
 *
 * @param lodmap		the LOD map for which to preload nodes
 * @param node			the quadtree node which we should preload
 */
static void preloadLodMapNode(OpenGLLodMap *lodmap, QuadtreeNode *node)
{
	OpenGLLodMapTile *tile = node->data;
	QuadtreeAABB box = quadtreeNodeAABB(node);

	switch(tile->status) {
		case OPENGL_LODMAP_TILE_ACTIVE:
			deactivateLodMapTile(tile);
		break;
		case OPENGL_LODMAP_TILE_INACTIVE:
		case OPENGL_LODMAP_TILE_META:
			LOG_DEBUG("Preloading LOD node covering [%d,%d]x[%d,%d] (LOD level %d) - %d threads active", box.minX, box.maxX, box.minY, box.maxY, node->level, g_thread_pool_get_num_threads(lodmap->loadingPool));
			tile->status = OPENGL_LODMAP_TILE_LOADING;
			g_thread_pool_push(lodmap->loadingPool, node, NULL);
		break;
		default:
			// nothing to do
		break;
	}

	if(!quadtreeNodeIsLeaf(node)) { // if this node has children, we can unload their data
		for(unsigned int i = 0; i < 4; i++) {
			QuadtreeNode *child = node->children[i];
			OpenGLLodMapTile *childTile = child->data;
			QuadtreeAABB childBox = quadtreeNodeAABB(child);

			switch(childTile->status) {
				case OPENGL_LODMAP_TILE_ACTIVE:
					LOG_DEBUG("Unloading LOD node covering [%d,%d]x[%d,%d] (LOD level %d)", childBox.minX, childBox.maxX, childBox.minY, childBox.maxY, child->level);
					deactivateLodMapTile(childTile);
					unloadLodMapTile(childTile);
				break;
				case OPENGL_LODMAP_TILE_READY:
					LOG_DEBUG("Unloading LOD node covering [%d,%d]x[%d,%d] (LOD level %d)", childBox.minX, childBox.maxX, childBox.minY, childBox.maxY, child->level);
					unloadLodMapTile(childTile);
				break;
				default:
					// nothing to do
				break;
			}
		}
	}
}

/**
 * Creates an LOD map tile
 *
 * @param tree			the quadtree for which to create the map tile
 * @param node			the quadtree node for which to create the map data
 */
static void createLodMapTile(Quadtree *tree, QuadtreeNode *node)
{
	OpenGLLodMapTile *tile = ALLOCATE_OBJECT(OpenGLLodMapTile);
	tile->status = OPENGL_LODMAP_TILE_INACTIVE;
	tile->mutex = g_mutex_new();
	tile->condition = g_cond_new();
	tile->heights = NULL;
	tile->heightsTexture = NULL;
	tile->normals = NULL;
	tile->normalsTexture = NULL;
	tile->texture = NULL;
	tile->textureTexture = NULL;
	tile->parentOffset = NULL;
	tile->minHeight = 0.0f;
	tile->maxHeight = 0.0f;
	tile->model = NULL;
	tile->drawOptions.drawMode = OPENGL_HEIGHTMAP_DRAW_NONE;

	node->data = tile;
}

/**
 * Activates a loaded LOD map tile by creating an OpenGL model with appropriate transform for it.
 *
 * Every node model is derived from the same basic heightmap grid (the one in lodmap->heightmap). While the 2D vertex coordinates range
 * from [0,tileSize]x[0,tileSize], they are actually scaled down to [0,1]x[0,1] by the vertex shader, which conveniently means that a
 * node of level n should be scaled by just n in both x and z directions during the transformation into world coordinates.
 * The transformation for the height values (or correspondingly the y direction) works differently: First, height values all lie in the
 * [0,1] range, usually originating from a grayscale image, and the node level stretch generally doesn't apply to them. However, the
 * data source provides us with a heightRatio parameter which we need to take into account and scale all levels with, no matter their
 * level in the tree.
 *
 * @param lodmap		the LOD map for which to active the tile
 * @param node			the quadtree node for which to activate the tile
 */
static void activateLodMapTile(OpenGLLodMap *lodmap, QuadtreeNode *node)
{
	OpenGLLodMapTile *tile = node->data;

	assert(tile->status == OPENGL_LODMAP_TILE_READY);

	// Create OpenGL textures
	tile->heightsTexture = createOpenGLVertexTexture2D(tile->heights);
	tile->heightsTexture->managed = false; // let us free the image
	tile->normalsTexture = createOpenGLTexture2D(tile->normals, false);
	tile->normalsTexture->internalFormat = GL_RGB16;
	tile->normalsTexture->wrappingMode = OPENGL_TEXTURE_WRAPPING_CLAMP;
	tile->normalsTexture->managed = false; // let us free the image
	initOpenGLTexture(tile->normalsTexture);
	synchronizeOpenGLTexture(tile->normalsTexture);
	tile->textureTexture = createOpenGLTexture2D(tile->texture, false);
	tile->textureTexture->wrappingMode = OPENGL_TEXTURE_WRAPPING_CLAMP;
	tile->textureTexture->managed = false; // let us free the image
	initOpenGLTexture(tile->textureTexture);
	synchronizeOpenGLTexture(tile->textureTexture);

	// Create OpenGL model
	tile->model = createOpenGLModel(lodmap->heightmap);
	attachOpenGLModelMaterial(tile->model, "lodmap");

	// Set model transform
	unsigned int scale = quadtreeNodeScale(node); // retrieve the scaling level from the node
	float *positionData = getVectorData(tile->model->translation);
	positionData[0] = node->x;
	positionData[1] = 0;
	positionData[2] = node->y; // y in model coordinates is z in world coordinates
	tile->model->scaleX = scale;
	tile->model->scaleY = lodmap->source->heightRatio;
	tile->model->scaleZ = scale;
	updateOpenGLModelTransform(tile->model);

	// At this point our tile is fully loaded except for the uniforms initialization, so let's initialize our parent now so we can grab its textures

	OpenGLLodMapTile *parentTile;
	tile->parentOffset = createVector2(0.0f, 0.0f);
	if(node->parent != NULL) {
		parentTile = node->parent->data;

		// wait until the node is fully loaded
		g_mutex_lock(parentTile->mutex);
		while(parentTile->status == OPENGL_LODMAP_TILE_LOADING) {
			g_cond_wait(parentTile->condition, parentTile->mutex);
		}
		g_mutex_unlock(parentTile->mutex);

		// activate if not active yet
		if(parentTile->status != OPENGL_LODMAP_TILE_ACTIVE) {
			activateLodMapTile(lodmap, node->parent);
		}

		// determine our index in our parent's node
		unsigned int parentIndex = quadtreeNodeGetParentContainingChildIndex(node);
		if(parentIndex & 1) { // second in x direction
			setVector(tile->parentOffset, 0, 0.5);
		}

		if(parentIndex & 2) { // second in y direction
			setVector(tile->parentOffset, 1, 0.5);
		}
	} else {
		parentTile = tile;
	}

	// Attach uniforms to model
	OpenGLUniformAttachment *uniforms = tile->model->uniforms;
	OpenGLUniform *heightsTextureUniform = getOpenGLUniform(uniforms, "heights");
	assert(heightsTextureUniform != NULL);
	heightsTextureUniform->content.texture_value = tile->heightsTexture;
	OpenGLUniform *normalsTextureUniform = getOpenGLUniform(uniforms, "normals");
	assert(normalsTextureUniform != NULL);
	normalsTextureUniform->content.texture_value = tile->normalsTexture;
	attachOpenGLUniform(uniforms, "texture", createOpenGLUniformTexture(tile->textureTexture));
	attachOpenGLUniform(uniforms, "parentNormals", createOpenGLUniformTexture(parentTile->normalsTexture));
	attachOpenGLUniform(uniforms, "parentTexture", createOpenGLUniformTexture(parentTile->textureTexture));
	attachOpenGLUniform(uniforms, "parentOffset", createOpenGLUniformVector(tile->parentOffset));
	attachOpenGLUniform(uniforms, "lodLevel", createOpenGLUniformInt(node->level));
	attachOpenGLUniform(uniforms, "enableFragmentMorph", createOpenGLUniformInt(parentTile == tile ? 0 : 1));

	// Make model invisible
	tile->model->visible = false;

	tile->status = OPENGL_LODMAP_TILE_ACTIVE;
}

/**
 * Deactivates an active LOD map tile
 *
 * @param tile			the LOD map tile to deactivate
 */
static void deactivateLodMapTile(OpenGLLodMapTile *tile)
{
	assert(tile->status == OPENGL_LODMAP_TILE_ACTIVE);

	freeOpenGLModel(tile->model);
	tile->model = NULL;

	freeOpenGLTexture(tile->heightsTexture);
	tile->heightsTexture = NULL;

	freeOpenGLTexture(tile->normalsTexture);
	tile->normalsTexture = NULL;

	freeOpenGLTexture(tile->textureTexture);
	tile->textureTexture = NULL;

	freeVector(tile->parentOffset);
	tile->parentOffset = NULL;

	tile->status = OPENGL_LODMAP_TILE_READY;
}

/**
 * Unloads a loaded LOD map tile
 *
 * @param tile			the LOD map tile to unload
 */
static void unloadLodMapTile(OpenGLLodMapTile *tile)
{
	assert(tile->status == OPENGL_LODMAP_TILE_READY);

	freeImage(tile->heights);
	freeImage(tile->normals);
	freeImage(tile->texture);

	tile->status = OPENGL_LODMAP_TILE_META;
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

	g_mutex_free(tile->mutex);
	g_cond_free(tile->condition);

	if(tile->status == OPENGL_LODMAP_TILE_ACTIVE) {
		deactivateLodMapTile(tile);
	}

	if(tile->status == OPENGL_LODMAP_TILE_READY) {
		freeImage(tile->heights);
		freeImage(tile->normals);
		freeImage(tile->texture);
	}

	free(tile);
}
