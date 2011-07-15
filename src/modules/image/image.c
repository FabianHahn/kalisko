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

#ifndef WIN32
#include <sys/types.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <glib.h>
#include <limits.h>
#include "dll.h"
#include "api.h"
#include "io.h"
#include "image.h"

MODULE_NAME("image");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a general image data type");
MODULE_VERSION(0, 5, 16);
MODULE_BCVERSION(0, 5, 16);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 10));

MODULE_INIT
{
	initImageIO();

	return true;
}

MODULE_FINALIZE
{
	freeImageIO();
}

/**
 * Creates a new byte image
 *
 * @param width			the width of the image to create
 * @param height		the height of the image to create
 * @param channels		the number of image channels to create
 * @result				the created image
 */
API Image *createImageByte(unsigned int width, unsigned int height, unsigned int channels)
{
	assert(channels > 0);

	Image *image = ALLOCATE_OBJECT(Image);
	image->width = width;
	image->height = height;
	image->channels = channels;
	image->type = IMAGE_TYPE_BYTE;
	image->data.byte_data = ALLOCATE_OBJECTS(unsigned char, width * height * channels);

	return image;
}

/**
 * Creates a new float image
 *
 * @param width			the width of the image to create
 * @param height		the height of the image to create
 * @param channels		the number of image channels to create
 * @result				the created image
 */
API Image *createImageFloat(unsigned int width, unsigned int height, unsigned int channels)
{
	assert(channels > 0);

	Image *image = ALLOCATE_OBJECT(Image);
	image->width = width;
	image->height = height;
	image->channels = channels;
	image->type = IMAGE_TYPE_FLOAT;
	image->data.float_data = ALLOCATE_OBJECTS(float, width * height * channels);

	return image;
}

/**
 * Creates a new image
 *
 * @param width			the width of the image to create
 * @param height		the height of the image to create
 * @param channels		the number of image channels to create
 * @param type			the type of the image to create
 * @result				the created image or NULL on failure
 */
API Image *createImage(unsigned int width, unsigned int height, unsigned int channels, ImageType type)
{
	Image *result = NULL;

	switch(type) {
		case IMAGE_TYPE_BYTE:
			result = createImageByte(width, height, channels);
		break;
		case IMAGE_TYPE_FLOAT:
			result = createImageFloat(width, height, channels);
		break;
		default:
			LOG_ERROR("Failed to create image: unsupported image type '%d'", type);
		break;
	}

	return result;
}

/**
 * Copies an image and possibly converts it to another type while doing so
 *
 * @param image			the image to copy
 * @param targetType	the type of the target image into which the contents of source should be copied
 */
API Image *copyImage(Image *source, ImageType targetType)
{
	Image *target;

	switch(targetType) {
		case IMAGE_TYPE_BYTE:
			target = createImageByte(source->width, source->height, source->channels);
		break;
		case IMAGE_TYPE_FLOAT:
			target = createImageFloat(source->width, source->height, source->channels);
		break;
		default:
			target = NULL;
	}

	for(unsigned int y = 0; y < source->height; y++) {
		for(unsigned int x = 0; x < source->width; x++) {
			for(unsigned int c = 0; c < source->channels; c++) {
				setImage(target, x, y, c, getImage(source, x, y, c));
			}
		}
	}

	return target;
}

/**
 * Clears an image by setting all its values to zero
 *
 * @param image			the image to clear
 */
API void clearImage(Image *image)
{
	unsigned int pixelSize = getImagePixelSize(image);
	void *data = getImageData(image);
	memset(data, 0, image->channels * image->height * image->width * pixelSize);
}

/**
 * Clears an image channel by setting its values to zero
 *
 * @param image			the image in which to clear a channel
 * @param channel		the channel that should be cleared
 */
API void clearImageChannel(Image *image, unsigned int channel)
{
	unsigned int pixelSize = getImagePixelSize(image);
	void *data = getImageData(image);
	memset(data + channel * image->height * image->width * pixelSize, 0, image->height * image->width * pixelSize);
}

/**
 * Normalize an image channel by shifting it linearly to the [0,1] range. Note that this only affects float images
 *
 * @param image			the image to normalize
 * @param channel		the image channel to normalize
 */
API void normalizeImageChannel(Image *image, unsigned int channel)
{
	if(image->type != IMAGE_TYPE_FLOAT) {
		return;
	}

	if(channel >= image->channels) {
		return;
	}


	// determine min / max values
	float minValue = FLT_MAX;
	float maxValue = FLT_MIN;

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			float value = getImageFloat(image, x, y, channel);

			if(value > maxValue) {
				maxValue = value;
			}

			if(value < minValue) {
				minValue = value;
			}
		}
	}

	LOG_DEBUG("Shifting image from [%f,%f] to [0,1]", minValue, maxValue);

	float factor = 1.0f / (maxValue - minValue);

	// shift the whole image
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			setImageFloat(image, x, y, channel, factor * (getImageFloat(image, x, y, channel) - minValue));
		}
	}
}

/**
 * Inverts an image channel
 *
 * @param image			the image to invert
 * @param channel		the image channel to invert
 */
API void invertImageChannel(Image *image, unsigned int channel)
{
	if(channel >= image->channels) {
		return;
	}

	// invert the image
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			setImage(image, x, y, channel, 1.0f - getImage(image, x, y, channel));
		}
	}
}

/**
 * Scales an image channel by multiplying it with a factor
 *
 * @param image			the image to scale
 * @param channel		the channel of the image to scale
 * @param factor		the factor to scale the image with
 */
API void scaleImageChannel(Image *image, unsigned int channel, float factor)
{
	if(channel >= image->channels || factor == 1.f) {
		return;
	}

	// scale the image
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			setImage(image, x, y, channel, factor * getImage(image, x, y, channel));
		}
	}
}

/**
 * Blends two images with a specified factor
 *
 * @param a			the first image to blend
 * @param b			the second image to blend
 * @param factor	the blending factor to use (i.e. the contribution of the first image)
 * @result			a new blended image using a's image type
 */
API Image *blendImages(Image *a, Image *b, double factor)
{
	if(a->channels != b->channels || a->width != b->width || a->height != b->height) {
		LOG_ERROR("Failed to blend images: Dimensions and channel counts must agree");
		return NULL;
	}

	Image *blend = createImage(a->width, a->height, a->channels, a->type);

	// blend the images
	for(unsigned int y = 0; y < a->height; y++) {
		for(unsigned int x = 0; x < a->width; x++) {
			for(unsigned int c = 0; c < a->channels; c++) {
				setImage(blend, x, y, c, factor * getImage(a, x, y, c) + (1.0 - factor) * getImage(b, x, y, c));
			}
		}
	}

	return blend;
}

/**
 * Saves an image in a quick-and-dirty way without having to specify any parameters, especially useful for debugging
 *
 * @param image			the image to debug
 */
API void debugImage(Image *image)
{
	if(!$$(bool, isModuleLoaded)("image_pnm")) {
		if(!$$(bool, requestModule)("image_pnm")) {
			LOG_DEBUG("Failed to save debug image: Failed to load module 'image_pnm'");
			return;
		}
	}

	for(unsigned int c = 0; c < image->channels; c++) { // normalize all channels
		normalizeImageChannel(image, c);
	}

	char *execpath = $$(char *, getExecutablePath)();
	GString *filename = g_string_new(execpath);
	g_string_append_printf(filename, "/debugimage.%d.%p.", getpid(), image);

	switch(image->channels) {
		case 1:
			g_string_append(filename, ".pgm");
		break;
		case 3:
			g_string_append(filename, ".ppm");
		break;
		default:
			g_string_append(filename, ".store");
		break;
	}

	LOG_DEBUG("Storing %u-channel debug image to '%s'", image->channels, filename->str);
	writeImageToFile(image, filename->str);
}

/**
 * Frees an image
 *
 * @param image			the image to free
 */
API void freeImage(Image *image)
{
	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			free(image->data.byte_data);
		break;
		case IMAGE_TYPE_FLOAT:
			free(image->data.float_data);
		break;
	}
	free(image);
}
