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

#ifndef LODMAP_SOURCE_H
#define LODMAP_SOURCE_H

#include "modules/image/image.h"

/**
 * Enum representing provider types for an OpenGL LOD map data source
 */
typedef enum {
	/** The data source provides no data of this type */
	OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE,
	/** The data source provides the leaf level of this type */
	OPENGL_LODMAP_DATASOURCE_PROVIDE_LEAF,
	/** The data source provides the whole pyramid of this type */
	OPENGL_LODMAP_DATASOURCE_PROVIDE_PYRAMID
} OpenGLLodMapDataSourceProviderType;

/**
 * Enum representing query types for an OpenGL LOD map data source
 */
typedef enum {
	/** Query for height data */
	OPENGL_LODMAP_DATASOURCE_QUERY_HEIGHT,
	/** Query for normal vector data */
	OPENGL_LODMAP_DATASOURCE_QUERY_NORMALS,
	/** Query for texture data */
	OPENGL_LODMAP_DATASOURCE_QUERY_TEXTURE
} OpenGLLodMapDataSourceQueryType;

struct OpenGLLodMapDataSourceStruct; // forward declaration

/**
 * Function pointer type for OpenGL LOD map data source loading callbacks
 */
typedef Image *(OpenGLLodMapDataSourceLoader)(struct OpenGLLodMapDataSourceStruct *dataSource, OpenGLLodMapDataSourceQueryType query, int x, int y, unsigned int level);

/**
 * Struct representing an OpenGL LOD map data source
 */
struct OpenGLLodMapDataSourceStruct {
	/** The size of a tile in pixels */
	unsigned int tileSize;
	/** The height provider type of this data source */
	OpenGLLodMapDataSourceProviderType providesHeight;
	/** The normal vector provider type of this data source */
	OpenGLLodMapDataSourceProviderType providesNormals;
	/** The texture provider type of this data source */
	OpenGLLodMapDataSourceProviderType providesTexture;
	/** The loader function callback of the data source */
	OpenGLLodMapDataSourceLoader *load;
};

typedef struct OpenGLLodMapDataSourceStruct OpenGLLodMapDataSource;

API Image *queryOpenGLLodMapDataSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapDataSourceQueryType query, int x, int y, unsigned int level);
API Image *queryOpenGLLodMapDataSourceHeight(OpenGLLodMapDataSource *dataSource, int x, int y, unsigned int level);
API Image *queryOpenGLLodMapDataSourceNormals(OpenGLLodMapDataSource *dataSource, int x, int y, unsigned int level);
API Image *queryOpenGLLodMapDataSourceTexture(OpenGLLodMapDataSource *dataSource, int x, int y, unsigned int level);

#endif
