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
 * Enum representing query types for an OpenGL LOD map data source
 */
typedef enum {
	/** Query for height data */
	OPENGL_LODMAP_IMAGE_HEIGHT,
	/** Query for normal vector data */
	OPENGL_LODMAP_IMAGE_NORMALS,
	/** Query for texture data */
	OPENGL_LODMAP_IMAGE_TEXTURE
} OpenGLLodMapImageType;

struct OpenGLLodMapDataSourceStruct; // forward declaration

/**
 * Function pointer type for OpenGL LOD map data source loading callbacks
 */
typedef Image *(OpenGLLodMapDataSourceLoader)(struct OpenGLLodMapDataSourceStruct *dataSource, OpenGLLodMapImageType query, int x, int y, unsigned int level, float *min, float *max);

/**
 * Function pointer type for OpenGL LOD map data source freeing callbacks
 */
typedef void (OpenGLLodMapDataSourceFreeFunction)(struct OpenGLLodMapDataSourceStruct *dataSource);

/**
 * Struct representing an OpenGL LOD map data source
 */
struct OpenGLLodMapDataSourceStruct {
	/** The base level of an LOD map tile */
	unsigned int baseLevel;
	/** The normal detail level offset of the data source */
	unsigned int normalDetailLevel;
	/** The texture detail level offset of the data source */
	unsigned int textureDetailLevel;
	/** The height ratio associated with this data source */
	float heightRatio;
	/** The data loader for this source */
	OpenGLLodMapDataSourceLoader *load;
	/**	The free function for this source */
	OpenGLLodMapDataSourceFreeFunction *free;
	/** Custom data for the data source */
	void *data;
};

typedef struct OpenGLLodMapDataSourceStruct OpenGLLodMapDataSource;

/**
 * Queries an OpenGL LOD map data source
 *
 * @param dataSource			the data source to query
 * @param query					the type of query to perform
 * @param x						the x position of the tile to query
 * @param y						the y position of the tile to query
 * @param level					the LOD level at which to perform the query
 * @param minValue				if not NULL, the minimum value of the looked up image will be written to the pointer target
 * @param maxValue				if not NULL, the maximum value of the looked up image will be written to the pointer target
 * @result						the result of the query
 */
static inline Image *queryOpenGLLodMapDataSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType query, int x, int y, unsigned int level, float *minValue, float *maxValue)
{
	return dataSource->load(dataSource, query, x, y, level, minValue, maxValue);
}

/**
 * Frees an OpenGL LOD map data source
 *
 * @param dataSource			the data source to free
 */
static inline void freeOpenGLLodMapDataSource(OpenGLLodMapDataSource *dataSource)
{
	dataSource->free(dataSource);
}

/**
 * Returns the tile size of a LOD map for a given data source
 *
 * @param type					the type of the tile for which to lookup the size
 * @param dataSource			the data source to check for the tile size
 * @result						the tile size of an LOD map tile
 */
static inline unsigned int getLodMapImageSize(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType type)
{
	unsigned int level = dataSource->baseLevel;

	if(type == OPENGL_LODMAP_IMAGE_NORMALS) {
		level += dataSource->normalDetailLevel;
	} else if(type == OPENGL_LODMAP_IMAGE_TEXTURE) {
		level += dataSource->textureDetailLevel;
	}

	return (1 << level) + 1;
}

#endif
