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
#define API
#include "lodmap.h"
#include "intersect.h"
#include "source.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 16, 2);
MODULE_BCVERSION(0, 14, 3);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("heightmap", 0, 4, 0), MODULE_DEPENDENCY("quadtree", 0, 11, 1), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 2, 0), MODULE_DEPENDENCY("linalg", 0, 3, 4), MODULE_DEPENDENCY("store", 0, 6, 11));

static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node, double time);
static void loadLodMapTile(Quadtree *tree, QuadtreeNode *node);
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
API OpenGLLodMap *createOpenGLLodMapFromStore(Store *store)
{
	Store *paramBaseRange = getStorePath(store, "lodmap/baseRange");
	if(paramBaseRange == NULL || !(paramBaseRange->type == STORE_INTEGER || paramBaseRange->type == STORE_FLOAT_NUMBER)) {
		LOG_ERROR("Failed to create LOD map from store: Required config float value 'lodmap/baseRange' not found!");
		return NULL;
	}

	double baseRange = paramBaseRange->type == STORE_FLOAT_NUMBER ? paramBaseRange->content.float_number : paramBaseRange->content.integer;

	Store *paramViewingDistance = getStorePath(store, "lodmap/viewingDistance");
	if(paramViewingDistance == NULL || paramViewingDistance->type != STORE_INTEGER) {
		LOG_ERROR("Failed to create LOD map from store: Required config integer value 'lodmap/viewingDistance' not found!");
		return NULL;
	}

	int viewingDistance = paramViewingDistance->content.integer;

	OpenGLLodMapDataSource *source = createOpenGLLodMapDataSourceFromStore(store);
	if(source == NULL) {
		LOG_ERROR("Failed to create LOD map from store: Failed to create LOD map data source!");
		return NULL;
	}

	OpenGLLodMap *lodmap = createOpenGLLodMap(source, baseRange, viewingDistance);
	if(lodmap == NULL) {
		freeOpenGLLodMapDataSource(source);
		return NULL;
	}

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
	lodmap->quadtree = createQuadtree(512, &loadLodMapTile, &freeLodMapTile, false);
	lodmap->selection = NULL;
	lodmap->viewerPosition = createVector3(0.0, 0.0, 0.0);
	lodmap->baseRange = baseRange;
	lodmap->viewingDistance = viewingDistance;
	lodmap->polygonMode = GL_FILL;
	lodmap->morphStartFactor = 0.8f;

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
	assignVector(lodmap->viewerPosition, position);

	// prune the quadtree to get rid of unused nodes
	pruneQuadtree(lodmap->quadtree);

	if(autoExpand) {
		// expand the quadtree to actually cover our position
		float *positionData = getVectorData(position);
		expandQuadtree(lodmap->quadtree, positionData[0], positionData[2]);
	}

	double range = getLodMapNodeRange(lodmap, lodmap->quadtree->root);
	QuadtreeAABB box = quadtreeNodeAABB(lodmap->quadtree->root);
	LOG_DEBUG("Updating LOD map for quadtree covering range [%d,%d]x[%d,%d] on %u levels", box.minX, box.maxX, box.minY, box.maxY, lodmap->quadtree->root->level);

	// free our previous selection list
	g_list_free(lodmap->selection);
	lodmap->selection = NULL;

	// select the LOD map nodes to be rendered
	if(lodmapQuadtreeNodeIntersectsSphere(lodmap->quadtree, lodmap->quadtree->root, position, range)) {
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
	updateQuadtreeNodeWeight(lodmap->quadtree->root); // the root could have been loaded by intersecting it
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
		drawOpenGLModel(tile->model, &tile->drawOptions);
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
static GList *selectLodMapNodes(OpenGLLodMap *lodmap, Vector *position, QuadtreeNode *node, double time)
{
	GList *nodes = NULL;

	node->time = time; // update access time

	double range = getLodMapNodeRange(lodmap, node);
	if(!lodmapQuadtreeNodeIntersectsSphere(lodmap->quadtree, node, position, range) && node->level > lodmap->viewingDistance) { // the node is outside its LOD viewing range and beyond the viewing distance, so cut it
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
			if(lodmapQuadtreeNodeIntersectsSphere(lodmap->quadtree, child, position, subrange)) { // the child node is insite its LOD viewing range for the current viewer position
				options.drawMode ^= drawMode; // don't draw this part if the child covers it

				GList *childNodes = selectLodMapNodes(lodmap, position, child, time); // node selection
				nodes = g_list_concat(nodes, childNodes); // append the child's nodes to our selection
			}
		}

		if(options.drawMode == OPENGL_HEIGHTMAP_DRAW_NONE) { // if all are, we're not drawn
			drawn = false;
		}

		// update node weight (we could have loaded child nodes)
		updateQuadtreeNodeWeight(node);
	}

	if(drawn) {
		assert(quadtreeNodeDataIsLoaded(node));

		OpenGLLodMapTile *tile = node->data;
		tile->drawOptions = options;

		nodes = g_list_append(nodes, node); // select ourselves
	}

	return nodes;
}

/**
 * Loads an LOD map tile by creating an OpenGL model with appropriate transform for it.
 *
 * Every node model is derived from the same basic heightmap grid (the one in lodmap->heightmap). While the 2D vertex coordinates range
 * from [0,tileSize]x[0,tileSize], they are actually scaled down to [0,1]x[0,1] by the vertex shader, which conveniently means that a
 * node of level n should be scaled by just n in both x and z directions during the transformation into world coordinates.
 * The transformation for the height values (or correspondingly the y direction) works differently: First, height values all lie in the
 * [0,1] range, usually originating from a grayscale image, and the node level stretch generally doesn't apply to them. However, the
 * data source provides us with a heightRatio parameter which we need to take into account and scale all levels with, no matter their
 * level in the tree.
 *
 * @param tree			the quadtree for which to load the map tile
 * @param node			the quadtree node for which to load the map data
 * @result				the loaded LOD map tile
 */
static void loadLodMapTile(Quadtree *tree, QuadtreeNode *node)
{
	OpenGLLodMap *lodmap = g_hash_table_lookup(maps, tree);
	assert(lodmap != NULL);

	OpenGLLodMapTile *tile = ALLOCATE_OBJECT(OpenGLLodMapTile);
	node->data = tile;

	tile->heights = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_HEIGHT, node->x, node->y, node->level, &tile->minHeight, &tile->maxHeight);
	tile->normals = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_NORMALS, node->x, node->y, node->level, NULL, NULL);
	tile->texture = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_TEXTURE, node->x, node->y, node->level, NULL, NULL);

	// apply correct scaling to the height limits date which will be used for 3D distance checks
	tile->minHeight *= lodmap->source->heightRatio;
	tile->maxHeight *= lodmap->source->heightRatio;

	// Create OpenGL textures
	tile->heightsTexture = createOpenGLVertexTexture2D(tile->heights);
	tile->normalsTexture = createOpenGLTexture2D(tile->normals, false);
	tile->normalsTexture->wrappingMode = OPENGL_TEXTURE_WRAPPING_CLAMP;
	initOpenGLTexture(tile->normalsTexture);
	synchronizeOpenGLTexture(tile->normalsTexture);
	tile->textureTexture = createOpenGLTexture2D(tile->texture, false);
	tile->textureTexture->wrappingMode = OPENGL_TEXTURE_WRAPPING_CLAMP;
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
		parentTile = loadQuadtreeNodeData(tree, node->parent, true);

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
	freeOpenGLModel(tile->model);
	freeOpenGLTexture(tile->heightsTexture);
	freeOpenGLTexture(tile->normalsTexture);
	freeOpenGLTexture(tile->textureTexture);
	freeVector(tile->parentOffset);
	free(tile);
}
