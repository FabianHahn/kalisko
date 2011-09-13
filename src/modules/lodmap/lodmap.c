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
#define API
#include "lodmap.h"
#include "intersect.h"
#include "source.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 11, 1);
MODULE_BCVERSION(0, 11, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("heightmap", 0, 4, 0), MODULE_DEPENDENCY("quadtree", 0, 11, 0), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("image_pnm", 0, 2, 6), MODULE_DEPENDENCY("image_png", 0, 1, 4), MODULE_DEPENDENCY("linalg", 0, 3, 4));

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

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(maps);
}

/**
 * Creates an OpenGL LOD map
 *
 * @param source				the data source used for the LOD map
 * @param baseRange				the base viewing range in world coordinates covered by the lowest LOD level in the LOD map
 * @param viewingDistance		the maximum viewing distance in LDO levels to be handled by this LOD map
 * @result						the created OpenGL LOD map
 */
API OpenGLLodMap *createOpenGLLodMap(OpenGLLodMapDataSource *source, double baseRange, unsigned int viewingDistance)
{
	unsigned int tileSize = getLodMapImageSize(source, OPENGL_LODMAP_IMAGE_HEIGHT);

	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->source = source;
	lodmap->heightmap = $(OpenGLPrimitive *, heightmap, createOpenGLPrimitiveHeightmap)(NULL, tileSize, tileSize); // create a managed heightmap that will serve as instance for our rendered tiles
	lodmap->quadtree = $(Quadtree *, quadtree, createQuadtree)(512, &loadLodMapTile, &freeLodMapTile, true);
	lodmap->selection = NULL;
	lodmap->viewerPosition = $(Vector *, linalg, createVector3)(0.0, 0.0, 0.0);
	lodmap->baseRange = baseRange;
	lodmap->viewingDistance = viewingDistance;
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
		$(void, quadtree, expandQuadtree)(lodmap->quadtree, positionData[0], positionData[2]);
	}

	double range = getLodMapNodeRange(lodmap, lodmap->quadtree->root);
	QuadtreeAABB box = quadtreeNodeAABB(lodmap->quadtree->root);
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
		// Now make sure the node data is loaded
		if(!quadtreeNodeDataIsLoaded(node)) {
			$(void *, quadtree, loadQuadtreeNodeData)(lodmap->quadtree, node, false); // load this node's data - our recursion parents will make sure they update their nodes' weights
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
static void loadLodMapTile(Quadtree *tree, QuadtreeNode *node)
{
	OpenGLLodMap *lodmap = g_hash_table_lookup(maps, tree);
	assert(lodmap != NULL);

	OpenGLLodMapTile *tile = ALLOCATE_OBJECT(OpenGLLodMapTile);
	node->data = tile;

	tile->heights = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_HEIGHT, node->x, node->y, node->level);
	tile->normals = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_NORMALS, node->x, node->y, node->level);
	tile->texture = queryOpenGLLodMapDataSource(lodmap->source, OPENGL_LODMAP_IMAGE_TEXTURE, node->x, node->y, node->level);

	// determine min/max heights
	unsigned int tileSize = getLodMapImageSize(lodmap->source, OPENGL_LODMAP_IMAGE_HEIGHT);
	tile->minHeight = FLT_MAX;
	tile->maxHeight = FLT_MIN;

	if(node->level == 0) { // retrieve min/max values from data
		for(unsigned int y = 1; y < tileSize + 1; y++) { // leave border away, only needed for base level interpolation!
			for(unsigned int x = 1; x < tileSize + 1; x++) { // leave border away, only needed for base level interpolation!
				float value = getImage(tile->heights, x, y, 0);

				// update min/max
				if(value < tile->minHeight) {
					tile->minHeight = value;
				}

				if(value > tile->maxHeight) {
					tile->maxHeight = value;
				}
			}
		}
	} else { // retrieve min/max values from children
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

	// At this point our tile is fully loaded except for the uniforms initialization, so let's initialize our parent now so we can grab its textures
	OpenGLLodMapTile *parentTile;
	tile->parentOffset = $(Vector *, linalg, createVector2)(0.0f, 0.0f);
	if(node->parent != NULL) {
		parentTile = $(void *, quadtree, loadQuadtreeNodeData)(tree, node->parent, true);

		// determine our index in our parent's node
		unsigned int parentIndex = quadtreeNodeGetParentContainingChildIndex(node);
		if(parentIndex & 1) { // second in x direction
			$(void, linalg, setVector)(tile->parentOffset, 0, 0.5);
		}

		if(parentIndex & 2) { // second in y direction
			$(void, linalg, setVector)(tile->parentOffset, 1, 0.5);
		}
	} else {
		parentTile = tile;
	}

	// Attach uniforms to model
	OpenGLUniformAttachment *uniforms = tile->model->uniforms;
	OpenGLUniform *heightsTextureUniform = $(OpenGLUniform *, opengl, getOpenGLUniform)(uniforms, "heights");
	assert(heightsTextureUniform != NULL);
	heightsTextureUniform->content.texture_value = tile->heightsTexture;
	OpenGLUniform *normalsTextureUniform = $(OpenGLUniform *, opengl, getOpenGLUniform)(uniforms, "normals");
	assert(normalsTextureUniform != NULL);
	normalsTextureUniform->content.texture_value = tile->normalsTexture;
	$(bool, opengl, attachOpenGLUniform)(uniforms, "texture", $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(tile->textureTexture));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "parentNormals", $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(parentTile->normalsTexture));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "parentTexture", $(OpenGLUniform *, opengl, createOpenGLUniformTexture)(parentTile->textureTexture));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "parentOffset", $(OpenGLUniform *, opengl, createOpenGLUniformVector)(tile->parentOffset));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "lodLevel", $(OpenGLUniform *, opengl, createOpenGLUniformInt)(node->level));
	$(bool, opengl, attachOpenGLUniform)(uniforms, "enableFragmentMorph", $(OpenGLUniform *, opengl, createOpenGLUniformInt)(parentTile == tile ? 0 : 1));

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
	$(void, opengl, freeOpenGLModel)(tile->model);
	$(void, opengl, freeOpenGLTexture)(tile->heightsTexture);
	$(void, opengl, freeOpenGLTexture)(tile->normalsTexture);
	$(void, opengl, freeOpenGLTexture)(tile->textureTexture);
	$(void, linalg, freeVector)(tile->parentOffset);
	free(tile);
}
