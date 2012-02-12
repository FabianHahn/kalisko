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
#include <glib.h>
#include "dll.h"
#include "modules/image/image.h"
#include "modules/heightmap/normals.h"
#define API
#include "source.h"
#include "imagesource.h"

typedef struct {
	/** The heights image associated with this image source */
	Image *heights;
	/** The normals image associated with this image source */
	Image *normals;
	/** The texture image associated with this image source */
	Image *texture;
	/** The LOD map data source for this image source */
	OpenGLLodMapDataSource source;
} OpenGLLodMapDataImageSource;

static void freeOpenGLLodMapImageSource(OpenGLLodMapDataSource *source);
static Image *queryOpenGLLodMapImageSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType query, int qx, int qy, unsigned int level, float *minValue, float *maxValue);
static Image *getImagePatch(Image *image, int sx, int sy, int size, unsigned int level, float *minValue, float *maxValue, bool interpolate);

/**
 * Creates an image source for an OpenGL LOD map
 *
 * @param heights				the image from which to read the height data (note that the source takes over control over this image, i.e. you must not free it)
 * @param normals				the image from which to read the normals data or NULL if unused (note that the source takes over control over this image, i.e. you must not free it)
 * @param texture				the image from which to read the texture data or NULL if unused (note that the source takes over control over this image, i.e. you must not free it)
 * @param baseLevel				the base level of a tile
 * @param heightRatio			the ratio to scale height values with
 * @result						the created LOD map data source or NULL on failure
 */
API OpenGLLodMapDataSource *createOpenGLLodMapImageSource(Image *heights, Image *normals, Image *texture, unsigned int baseLevel, float heightRatio)
{
	int normalDetailLevel = 0;
	int textureDetailLevel = 0;

	if(normals != NULL) { // Check provided heights image
		double widthRatio = (double) normals->width / heights->width;
		double heightRatio = (double) normals->height / heights->height;

		if(widthRatio != heightRatio || widthRatio < 1.0) {
			LOG_ERROR("Failed to create LOD map image source: Normals image must have the same aspect ratio as heights image and be at least as large");
			return NULL;
		}

		unsigned int currentWidth = heights->width;

		while(currentWidth < normals->width) {
			currentWidth *= 2;
			normalDetailLevel++;
		}

		unsigned int scaleFactor = 1 << normalDetailLevel;

		if((scaleFactor * heights->width != normals->width) || (scaleFactor * heights->height != normals->height)) {
			LOG_ERROR("Failed to create LOD map image source: Normal detail image must be exactly 2 to the power of X times larger than heights image");
			return NULL;
		}
	}

	if(texture != NULL) { // Check provided texture image
		double widthRatio = (double) texture->width / heights->width;
		double heightRatio = (double) texture->height / heights->height;

		if(widthRatio != heightRatio || widthRatio < 1.0) {
			LOG_ERROR("Failed to create LOD map image source: Texture image must have the same aspect ratio as heights image and be at least as large");
			return NULL;
		}

		unsigned int currentWidth = heights->width;

		while(currentWidth < texture->width) {
			currentWidth *= 2;
			textureDetailLevel++;
		}

		unsigned int scaleFactor = 1 << textureDetailLevel;

		if((scaleFactor * heights->width != texture->width) || (scaleFactor * heights->height != texture->height)) {
			LOG_ERROR("Failed to create LOD map image source: Texture detail image must be exactly 2 to the power of X times larger than heights image");
			return NULL;
		}
	}

	if(normals == NULL) { // Compute normals if not provided
		normals = createImage(heights->width, heights->height, 3, IMAGE_TYPE_FLOAT);
		computeHeightmapNormals(heights, normals);
	}

	if(texture == NULL) { // Clear texture if not provided
		texture = createImage(heights->width, heights->height, 3, IMAGE_TYPE_FLOAT);
		clearImage(texture);
	}

	OpenGLLodMapDataImageSource *source = ALLOCATE_OBJECT(OpenGLLodMapDataImageSource);
	source->heights = heights;
	source->normals = normals;
	source->texture = texture;
	source->source.baseLevel = baseLevel;
	source->source.normalDetailLevel = normalDetailLevel;
	source->source.textureDetailLevel = textureDetailLevel;
	source->source.heightRatio = heightRatio;
	source->source.load = &queryOpenGLLodMapImageSource;
	source->source.free = &freeOpenGLLodMapImageSource;
	source->source.data = source;

	return &source->source;
}

/**
 * Frees an image source for an OpenGL LOD map
 *
 * @param source			the image source to free
 */
static void freeOpenGLLodMapImageSource(OpenGLLodMapDataSource *source)
{
	OpenGLLodMapDataImageSource *imageSource = source->data;

	freeImage(imageSource->heights);
	freeImage(imageSource->texture);
	free(imageSource);
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
static Image *queryOpenGLLodMapImageSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapImageType query, int qx, int qy, unsigned int level, float *minValue, float *maxValue)
{
	OpenGLLodMapDataImageSource *imageSource = dataSource->data;
	int imageSize = getLodMapImageSize(dataSource, query);
	Image *image = NULL;
	bool interpolate = true;
	float scale = 1.0f;
	float minValueBuffer = 0.0f;
	float maxValueBuffer = 0.0f;

	switch(query) {
		case OPENGL_LODMAP_IMAGE_HEIGHT:
			image = imageSource->heights;
			interpolate = false;
			scale = (1.0f / imageSize) * dataSource->heightRatio;
		break;
		case OPENGL_LODMAP_IMAGE_NORMALS:
			image = imageSource->normals;
			scale = 1 << level;
		break;
		case OPENGL_LODMAP_IMAGE_TEXTURE:
			image = imageSource->texture;
		break;
	}

	Image *result = getImagePatch(image, qx * (imageSize - 1), qy * (imageSize - 1), imageSize, level, &minValueBuffer, &maxValueBuffer, interpolate);
	scaleImageChannel(result, 0, scale);
	scaleImageChannel(result, 2, scale);

	if(minValue != NULL) {
		*minValue = minValueBuffer * scale;
	}
	if(maxValue != NULL) {
		*maxValue = maxValueBuffer * scale;
	}

	return result;
}

/**
 * Retrieves a patch of an image at a certain pyramid level by interpolating or propagating its values from lower levels
 *
 * @param image			the root image in which to search
 * @param sx			the x coordinate of the top left point of the image patch to extract
 * @param sy			the y coordinate of the top left point of the image patch to extract
 * @param size			the size of the image patch to extract
 * @param level			the pyramid level at which to extract the patch
 * @param interpolate	specifies whether image values should be interpolated or propagated
 * @param minValue		the minimum value of the looked up image will be written to the pointer target
 * @param maxValue		the maximum value of the looked up image will be written to the pointer target
 * @result				the extracted patch
 */
static Image *getImagePatch(Image *image, int sx, int sy, int size, unsigned int level, float *minValue, float *maxValue, bool interpolate)
{
	assert(minValue != NULL);
	assert(maxValue != NULL);

	*minValue = FLT_MAX;
	*maxValue = FLT_MIN;

	Image *result = createImageFloat(size, size, image->channels);

	if(level == 0) {
		for(int y = sy; y < sy + size; y++) {
			for(int x = sx; x < sx + size; x++) {
				for(unsigned int c = 0; c < image->channels; c++) {
					float value;
					if(y >= 0 && y < image->height && x >= 0 && x < image->width) {
						value = getImage(image, x, y, c);
					} else {
						value = 0.0f;
					}

					setImage(result, x - sx, y - sy, c, value);

					// update min/max
					if(value < *minValue) {
						*minValue = value;
					}
					if(value > *maxValue) {
						*maxValue = value;
					}
				}
			}
		}
	} else {
		if(interpolate) { // interpolate 8-neighborhood for each pixel
			int detailStep = 1 << (level - 1);
			Image *detail = getImagePatch(image, sx - detailStep, sy - detailStep, 2 * (size - 1) + 3, level - 1, minValue, maxValue, interpolate);
			*minValue = FLT_MAX;
			*maxValue = FLT_MIN;

			for(int y = 0; y < size; y++) {
				int dy = 2 * y;
				for(int x = 0; x < size; x++) {
					int dx = 2 * x;
					for(unsigned int c = 0; c < image->channels; c++) {
						float value = 0.0f;
						for(int i = -1; i <= 1; i++) {
							for(int j = -1; j <= 1; j++) {
								value += getImage(detail, dx + 1 + j, dy + 1 + i, c);
							}
						}
						value /= 9.0f;

						setImage(result, x, y, c, value);

						// update min/max
						if(value < *minValue) {
							*minValue = value;
						}
						if(value > *maxValue) {
							*maxValue = value;
						}
					}
				}
			}

			freeImage(detail);
		} else { // just propagate the exact pixel from the lower level
			Image *detail = getImagePatch(image, sx, sy, 2 * (size - 1) + 1, level - 1, minValue, maxValue, interpolate);
			*minValue = FLT_MAX;
			*maxValue = FLT_MIN;

			for(int y = 0; y < size; y++) {
				int dy = 2 * y;
				for(int x = 0; x < size; x++) {
					int dx = 2 * x;
					for(unsigned int c = 0; c < image->channels; c++) {
						float value = getImage(detail, dx, dy, c);

						setImage(result, x, y, c, value);

						// update min/max
						if(value < *minValue) {
							*minValue = value;
						}
						if(value > *maxValue) {
							*maxValue = value;
						}
					}
				}
			}

			freeImage(detail);
		}
	}

	return result;
}

