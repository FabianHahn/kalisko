/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
#include <glib.h>
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#define API
#include "source.h"
#include "importsource.h"

typedef struct {
	/** The the folder path in which the import source is stored */
	char *path;
	/** The LOD map data source for this image source */
	OpenGLLodMapDataSource source;
} OpenGLLodMapDataImportSource;

static void freeOpenGLLodMapImportSource(OpenGLLodMapDataSource *source);
static Image *queryOpenGLLodMapImportSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType query, int qx, int qy, unsigned int level, float *minValue, float *maxValue);

/**
 * Creates an import source for an OpenGL LOD map from a store configuration
 *
 * @param store			the store configuration from which to create the OpenGL LOD map import data source
 * @result				the created data source or NULL on failure
 */
API OpenGLLodMapDataSource *createOpenGLLodMapImportSourceFromStore(Store *store)
{
	Store *pathParam = getStorePath(store, "lodmap/source/path");
	if(pathParam == NULL || pathParam->type != STORE_STRING) {
		LOG_ERROR("Failed to create LOD map import source: Config integer value 'lodmap/source/path' not found!");
		return NULL;
	}

	// load baseRange parameter
	Store *baseLevelParam = getStorePath(store, "lodmap/source/baseLevel");
	if(baseLevelParam == NULL || baseLevelParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to create LOD map import source: Config integer value 'lodmap/source/baseLevel' not found!");
		return NULL;
	}

	// load normalDetailLevel parameter
	Store *normalDetailLevelParam = getStorePath(store, "lodmap/source/normalDetailLevel");
	if(normalDetailLevelParam == NULL || normalDetailLevelParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to create LOD map import source: Config integer value 'lodmap/source/normalDetailLevel' not found!");
		return NULL;
	}

	// load textureDetailLevel parameter
	Store *textureDetailLevelParam = getStorePath(store, "lodmap/source/textureDetailLevel");
	if(textureDetailLevelParam == NULL || textureDetailLevelParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to create LOD map import source: Config integer value 'lodmap/source/textureDetailLevel' not found!");
		return NULL;
	}

	// load heightRatio parameter
	Store *heightRatioParam = getStorePath(store, "lodmap/source/heightRatio");
	if(heightRatioParam == NULL || !(heightRatioParam->type == STORE_INTEGER || heightRatioParam->type == STORE_FLOAT_NUMBER)) {
		LOG_ERROR("Failed to create LOD map import source: Config float value 'lodmap/source/heightRatio' not found!");
		return NULL;
	}

	// create the struct for the image source
	OpenGLLodMapDataImportSource *source = ALLOCATE_OBJECT(OpenGLLodMapDataImportSource);
	source->path = strdup(pathParam->content.string);
	source->source.baseLevel = baseLevelParam->content.integer;
	source->source.normalDetailLevel = normalDetailLevelParam->content.integer;
	source->source.textureDetailLevel = textureDetailLevelParam->content.integer;
	source->source.heightRatio = heightRatioParam->type == STORE_FLOAT_NUMBER ? heightRatioParam->content.float_number : heightRatioParam->content.integer;
	source->source.load = &queryOpenGLLodMapImportSource;
	source->source.free = &freeOpenGLLodMapImportSource;
	source->source.data = source;

	return &source->source;
}

/**
 * Creates an import source for an OpenGL LOD map
 *
 * @param path			the path from which to configure the import data source
 * @result				the created LOD map data source or NULL on failure
 */
API OpenGLLodMapDataSource *createOpenGLLodMapImportSource(const char *path)
{
	GString *metaPath = g_string_new(path);
	g_string_append(metaPath, "/lodmap.store");
	Store *store = parseStoreFile(metaPath->str);

	if(store == NULL) {
		LOG_ERROR("Failed to create LOD map import source: Failed to load configuration store file from '%s'", metaPath->str);
		g_string_free(metaPath, true);
		return NULL;
	}

	g_string_free(metaPath, true);

	OpenGLLodMapDataSource *source = createOpenGLLodMapImportSourceFromStore(store);
	freeStore(store);

	return source;
}

/**
 * Frees an image source for an OpenGL LOD map
 *
 * @param source			the image source to free
 */
static void freeOpenGLLodMapImportSource(OpenGLLodMapDataSource *source)
{
	OpenGLLodMapDataImportSource *importSource = source->data;

	free(importSource->path);
	free(importSource);
}

/**
 * Queries an OpenGL LOD map image data source
 *
 * @param dataSource			the data source to query
 * @param query					the type of query to perform
 * @param qx					the x position of the tile to query
 * @param qy					the y position of the tile to query
 * @param level					the LOD level at which to perform the query
 * @param minValue				if not NULL, the minimum value of the looked up image will be written to the pointer target
 * @param maxValue				if not NULL, the maximum value of the looked up image will be written to the pointer target
 * @result						the result of the query
 */
static Image *queryOpenGLLodMapImportSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType query, int qx, int qy, unsigned int level, float *minValue, float *maxValue)
{
	OpenGLLodMapDataImportSource *importSource = dataSource->data;
	int imageSize = getLodMapImageSize(dataSource, query);

	GString *imageName = g_string_new(importSource->path);
	unsigned channels = 0;

	switch(query) {
		case OPENGL_LODMAP_IMAGE_HEIGHT:
			g_string_append_printf(imageName, "/lodmap_heights_%u.%d.%d.png", level, qx, qy);
			channels = 1;
		break;
		case OPENGL_LODMAP_IMAGE_NORMALS:
			g_string_append_printf(imageName, "/lodmap_normals_%u.%d.%d.png", level, qx, qy);
			channels = 3;
		break;
		case OPENGL_LODMAP_IMAGE_TEXTURE:
			g_string_append_printf(imageName, "/lodmap_texture_%u.%d.%d.png", level, qx, qy);
			channels = 3;
		break;
	}

	Image *image = readImageFromFile(imageName->str);
	g_string_free(imageName, true);

	if(image == NULL) {
		image = createImageFloat(imageSize, imageSize, channels);
		clearImage(image);
	}

	if(image->width != imageSize || image->height != imageSize || image->channels != channels) {
		freeImage(image);
		image = createImageFloat(imageSize, imageSize, channels);
		clearImage(image);
	}

	// determine min/max
	if(minValue != NULL || maxValue != NULL) {
		if(minValue != NULL) {
			*minValue = FLT_MAX;
		}

		if(maxValue != NULL) {
			*maxValue = -FLT_MAX;
		}

		for(unsigned int y = 0; y < image->height; y++) {
			for(unsigned int x = 0; x < image->width; x++) {
				for(unsigned int c = 0; c < image->channels; c++) {
					float value = getImage(image, x, y, c);

					// update min/max
					if(minValue != NULL && value < *minValue) {
						*minValue = value;
					}
					if(maxValue != NULL && value > *maxValue) {
						*maxValue = value;
					}
				}
			}
		}
	}

	return image;
}
