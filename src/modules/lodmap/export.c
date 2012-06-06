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

#include <glib.h>
#include "dll.h"
#include "modules/quadtree/quadtree.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#define API
#include "lodmap.h"
#include "source.h"
#include "export.h"

static void exportOpenGLLodMapQuadtreeNode(OpenGLLodMap *lodmap, QuadtreeNode *node, const char *path, Store *meta);

/**
 * Exports an OpenGL LOD map to a folder by dumping both the metadata and the node data into files
 *
 * @param lodmap		the OpenGL LOD map to export
 * @param path			the target folder to export to
 * @result				true if successful
 */
bool exportOpenGLLodMap(OpenGLLodMap *lodmap, const char *path)
{
	if(g_mkdir_with_parents(path, 0755) < 0) {
		LOG_SYSTEM_ERROR("Failed to create LOD map export folder at '%s':", path);
		return false;
	}

	// First dump config information
	Store *config = createStore();
	setStorePath(config, "lodmap", createStore());
	setStorePath(config, "lodmap/baseRange", createStoreFloatNumberValue(lodmap->baseRange));
	setStorePath(config, "lodmap/viewingDistance", createStoreIntegerValue(lodmap->baseRange));

	OpenGLLodMapDataSource *source = lodmap->source;
	setStorePath(config, "lodmap/source", createStore());
	setStorePath(config, "lodmap/source/baseLevel", createStoreIntegerValue(source->baseLevel));
	setStorePath(config, "lodmap/source/normalDetailLevel", createStoreIntegerValue(source->normalDetailLevel));
	setStorePath(config, "lodmap/source/textureDetailLevel", createStoreIntegerValue(source->textureDetailLevel));
	setStorePath(config, "lodmap/source/heightRatio", createStoreFloatNumberValue(source->heightRatio));

	setStorePath(config, "lodmap/quadtree", createStore());
	setStorePath(config, "lodmap/quadtree/rootX", createStoreIntegerValue(lodmap->quadtree->root->x));
	setStorePath(config, "lodmap/quadtree/rootY", createStoreIntegerValue(lodmap->quadtree->root->y));
	setStorePath(config, "lodmap/quadtree/rootLevel", createStoreIntegerValue(lodmap->quadtree->root->level));

	Store *meta = createStore();
	setStorePath(config, "lodmap/meta", meta);

	// Now dump the tree contents
	exportOpenGLLodMapQuadtreeNode(lodmap, lodmap->quadtree->root, path, meta);

	GString *metaPath = g_string_new(path);
	g_string_append(metaPath, "/lodmap.store");
	writeStoreFile(metaPath->str, config);
	g_string_free(metaPath, true);

	freeStore(config);

	return true;
}

/**
 * Recursively dumps an OpenGL LOD map quadtree node and all its children into files at a specified path
 *
 * @param lodmap		the OpenGL LOD map to which this node belongs
 * @param node			the current quadtree node we're visiting during the recursion
 * @param path			the target folder to export to
 * @param meta			the current meta store to write metadata about this node to
 */
static void exportOpenGLLodMapQuadtreeNode(OpenGLLodMap *lodmap, QuadtreeNode *node, const char *path, Store *meta)
{
	OpenGLLodMapTile *tile = node->data;

	setStorePath(meta, "minHeight", createStoreFloatNumberValue(tile->minHeight));
	setStorePath(meta, "maxHeight", createStoreFloatNumberValue(tile->maxHeight));
	setStorePath(meta, "children", createStoreListValue(NULL));

	// export heights
	GString *heightsName = g_string_new(path);
	g_string_append_printf(heightsName, "/lodmap_heights_%u.%d.%d.png", node->level, node->x, node->y);

	if(!writeImageToFile(tile->heights, heightsName->str)) {
		LOG_ERROR("Failed to export LOD map heights image to '%s'", heightsName->str);
	}

	g_string_free(heightsName, true);

	// export normals
	GString *normalsName = g_string_new(path);
	g_string_append_printf(normalsName, "/lodmap_normals_%u.%d.%d.png", node->level, node->x, node->y);

	if(!writeImageToFile(tile->normals, normalsName->str)) {
		LOG_ERROR("Failed to export LOD map normals image to '%s'", normalsName->str);
	}

	g_string_free(normalsName, true);

	// export texture
	GString *textureName = g_string_new(path);
	g_string_append_printf(textureName, "/lodmap_texture_%u.%d.%d.png", node->level, node->x, node->y);

	if(!writeImageToFile(tile->texture, textureName->str)) {
		LOG_ERROR("Failed to export LOD map texture image to '%s'", textureName->str);
	}

	g_string_free(textureName, true);

	// Recursively export children
	if(!quadtreeNodeIsLeaf(node)) {
		for(unsigned int i = 0; i < 4; i++) {
			Store *childMeta = createStore();
			setStorePath(meta, "children/%d", childMeta, i);
			exportOpenGLLodMapQuadtreeNode(lodmap, node->children[i], path, childMeta);
		}
	}

	LOG_INFO("Exported LOD map quadtree node (%d,%d) at level %u", node->x, node->y, node->level);
}
