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
#include "api.h"
#include "lodmap.h"

MODULE_NAME("lodmap");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL level-of-detail maps");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 27, 0), MODULE_DEPENDENCY("heightmap", 0, 2, 13), MODULE_DEPENDENCY("quadtree", 0, 4, 2), MODULE_DEPENDENCY("image", 0, 5, 14));

void *loadLodMapTile(Quadtree *tree, double x, double y);
void freeLodMapTile(Quadtree *tree, void *data);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Creates an OpenGL LOD map primitive
 *
 * @result			the created OpenGL LOD map primitve
 */
API OpenGLPrimitive *createOpenGLPrimitiveLodMap()
{
	OpenGLLodMap *lodmap = ALLOCATE_OBJECT(OpenGLLodMap);
	lodmap->primitive.type = "lodmap";
	lodmap->primitive.data = lodmap;
	lodmap->primitive.setup_function = NULL;
	lodmap->primitive.draw_function = NULL;
	lodmap->primitive.update_function = NULL;
	lodmap->primitive.free_function = &freeOpenGLPrimitiveLodMap;

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
	free(lodmap);
}

/**
 * Loads an LOD map tile
 *
 * @param tree			the quadtree for which to load the map tile
 * @param x				the x coordinate of the LOD map tile to load
 * @parma y				the y coordinate of the LOD map tile to load
 * @result				the loaded LOD map tile
 */
void *loadLodMapTile(Quadtree *tree, double x, double y)
{
	OpenGLLodMapTile *tile = ALLOCATE_OBJECT(OpenGLLodMapTile);

	// todo: quadtree lookup

	return tile;
}

/**
 * Frees an LOD map tile
 *
 * @param tree			the quadtree for which to free the map tile
 * @param data			the LOD map tile to free
 */
void freeLodMapTile(Quadtree *tree, void *data)
{
	OpenGLLodMapTile *tile = data;
	free(tile);
}
