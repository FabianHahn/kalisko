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
#include "modules/image/image.h"
#include "modules/heightmap/normals.h"
#define API
#include "source.h"
#include "sources.h"

static Image *queryOpenGLLodMapImageSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapDataSourceQueryType query, int qx, int qy, unsigned int level);
static Image *getImagePatch(Image *image, int sx, int sy, int size, unsigned int level);

/**
 * Creates a null source for an OpenGL LOD map, i.e. a source that doesn't provide any data at all
 *
 * @param baseLevel				the base level of a tile for the null source
 * @result						the created LOD map data source
 */
API OpenGLLodMapDataSource *createOpenGLLodMapNullSource(unsigned int baseLevel)
{
	OpenGLLodMapDataSource *source = ALLOCATE_OBJECT(OpenGLLodMapDataSource);
	source->baseLevel = baseLevel;
	source->providesHeight = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->providesNormals = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->providesTexture = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->load = NULL; // if we don't provide anything, we will never be called

	return source;
}

/**
 * Creates an image source for an OpenGL LOD map
 *
 * @param heights				the image from which to read the height data (note that the source takes over control over this image, i.e. you must not free it)
 * @param texture				the image from which to read the texture data (note that the source takes over control over this image, i.e. you must not free it)
 * @param baseLevel				the base level of a tile for the null source
 * @result						the created LOD map data source
 */
API OpenGLLodMapDataImageSource *createOpenGLLodMapImageSource(Image *heights, Image *texture, unsigned int baseLevel, float heightRatio)
{
	OpenGLLodMapDataImageSource *source = ALLOCATE_OBJECT(OpenGLLodMapDataImageSource);
	source->heights = heights;
	source->normals = createImageFloat(heights->width, heights->height, 3);
	computeHeightmapNormals(source->heights, source->normals);
	source->texture = texture;
	source->heightRatio = heightRatio;
	source->source.baseLevel = baseLevel;
	source->source.providesHeight = OPENGL_LODMAP_DATASOURCE_PROVIDE_LEAF;
	source->source.providesNormals = OPENGL_LODMAP_DATASOURCE_PROVIDE_PYRAMID;
	source->source.providesTexture = OPENGL_LODMAP_DATASOURCE_PROVIDE_PYRAMID;
	source->source.load = &queryOpenGLLodMapImageSource;
	source->source.data = source;

	return source;
}

/**
 * Frees an image source for an OpenGL LOD map
 *
 * @param source			the image source to free
 */
API void freeOpenGLLodMapImageSource(OpenGLLodMapDataImageSource *source)
{
	$(void, image, freeImage)(source->heights);
	$(void, image, freeImage)(source->texture);
	free(source);
}

/**
 * Queries an OpenGL LOD map image data source
 *
 * @param dataSource			the data source to query
 * @param query					the type of query to perform
 * @param qx					the x position of the tile to query
 * @param qy					the y position of the tile to query
 * @param level					the LOD level at which to perform the query
 * @result						the result of the query or NULL if the source doesn't provide data for this kind of query
 */
static Image *queryOpenGLLodMapImageSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapDataSourceQueryType query, int qx, int qy, unsigned int level)
{
	int tileSize = getLodMapTileSize(dataSource);
	OpenGLLodMapDataImageSource *imageSource = dataSource->data;
	Image *result;

	float heightScale = (1.0f / tileSize) * imageSource->heightRatio;

	switch(query) {
		case OPENGL_LODMAP_DATASOURCE_QUERY_HEIGHT:
			result = $(Image *, image, createImageFloat)(tileSize + 2, tileSize + 2, 1);
			for(int y = 0; y < tileSize + 2; y++) {
				int dy = qy * (tileSize - 1) - 1 + y;
				for(int x = 0; x < tileSize + 2; x++) {
					int dx = qx * (tileSize - 1) - 1 + x;

					if(dy >= 0 && dy < imageSource->heights->height && dx >= 0 && dx < imageSource->heights->width) {
						setImage(result, x, y, 0, heightScale * getImage(imageSource->heights, dx, dy, 0));
					} else {
						setImage(result, x, y, 0, 0.0);
					}
				}
			}
		break;
		case OPENGL_LODMAP_DATASOURCE_QUERY_NORMALS:
			result = getImagePatch(imageSource->normals, qx * (tileSize - 1) - 1, qy * (tileSize - 1) - 1, tileSize + 2, level);
		break;
		case OPENGL_LODMAP_DATASOURCE_QUERY_TEXTURE:
			result = getImagePatch(imageSource->texture, qx * (tileSize - 1), qy * (tileSize - 1), tileSize, level);
		break;
		default:
			return NULL;
		break;
	}

	return result;
}

/**
 * Retrieves a patch of an image at a certain pyramid level by interpolating its values from lower levels
 *
 * @param image			the root image in which to search
 * @param sx			the x coordinate of the top left point of the image patch to extract
 * @param sy			the y coordinate of the top left point of the image patch to extract
 * @param size			the size of the image patch to extract
 * @param level			the pyramid level at which to extract the patch
 * @result				the extracted patch
 */
static Image *getImagePatch(Image *image, int sx, int sy, int size, unsigned int level)
{
	Image *result = createImageFloat(size, size, image->channels);

	if(level == 0) {
		for(int y = sy; y < sy + size; y++) {
			for(int x = sx; x < sx + size; x++) {
				for(unsigned int c = 0; c < image->channels; c++) {
					if(y >= 0 && y < image->height && x >= 0 && x < image->width) {
						setImage(result, x - sx, y - sy, c, getImage(image, x, y, c));
					} else {
						setImage(result, x - sx, y - sy, c, 0.0);
					}
				}
			}
		}
	} else {
		int detailStep = 1 << (level - 1);
		Image *detail = getImagePatch(image, sx - detailStep, sy - detailStep, 2 * (size - 1) + 3, level - 1);

		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				for(unsigned int c = 0; c < image->channels; c++) {
					float value = 0.0f;
					for(int i = -1; i <= 1; i++) {
						for(int j = -1; j <= 1; j++) {
							value += getImage(detail, 2 * x + 1 + j, 2 * y + 1 + i, c);
						}
					}
					value /= 9.0f;
					setImage(result, x, y, c, value);
				}
			}
		}

		freeImage(detail);
	}

	return result;
}
