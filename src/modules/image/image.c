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

#ifdef WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <glib.h>
#include <limits.h>
#include "dll.h"
#define API
#include "io.h"
#include "image.h"

MODULE_NAME("image");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a general image data type");
MODULE_VERSION(0, 5, 20);
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
			logError("Failed to create image: unsupported image type '%d'", type);
		break;
	}

	return result;
}

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

API void clearImage(Image *image)
{
	unsigned int pixelSize = getImagePixelSize(image);
	void *data = getImageData(image);
	memset(data, 0, image->channels * image->height * image->width * pixelSize);
}

API void clearImageChannel(Image *image, unsigned int channel)
{
	unsigned int pixelSize = getImagePixelSize(image);
	void *data = getImageData(image);
	memset(data + channel * image->height * image->width * pixelSize, 0, image->height * image->width * pixelSize);
}

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

	logInfo("Shifting image from [%f,%f] to [0,1]", minValue, maxValue);

	float factor = 1.0f / (maxValue - minValue);

	// shift the whole image
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			setImageFloat(image, x, y, channel, factor * (getImageFloat(image, x, y, channel) - minValue));
		}
	}
}

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

API Image *blendImages(Image *a, Image *b, double factor)
{
	if(a->channels != b->channels || a->width != b->width || a->height != b->height) {
		logError("Failed to blend images: Dimensions and channel counts must agree");
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

API Image *flipImage(Image *image, int flipModes)
{
	Image *result = copyImage(image, image->type);

	// flip the image
	for(unsigned int y = 0; y < image->height; y++) {
		unsigned int ty = (flipModes & IMAGE_FLIP_Y) ? image->height - y - 1 : y;
		for(unsigned int x = 0; x < image->width; x++) {
			unsigned int tx = (flipModes & IMAGE_FLIP_X) ? image->width - x - 1 : x;
			for(unsigned int c = 0; c < image->channels; c++) {
				setImage(result, tx, ty, c, getImage(image, x, y, c));
			}
		}
	}

	return result;
}

API void debugImage(Image *image)
{
	if(!$$(bool, isModuleLoaded)("image_pnm")) {
		if(!$$(bool, requestModule)("image_pnm")) {
			logInfo("Failed to save debug image: Failed to load module 'image_pnm'");
			return;
		}
	}

	for(unsigned int c = 0; c < image->channels; c++) { // normalize all channels
		normalizeImageChannel(image, c);
	}

	GString *filename = g_string_new("debugimage");
	g_string_append_printf(filename, ".%d.%p.", getpid(), image);

	switch(image->channels) {
		case 1:
			g_string_append(filename, "pgm");
		break;
		case 3:
			g_string_append(filename, "ppm");
		break;
		default:
			g_string_append(filename, "store");
		break;
	}

	logInfo("Storing %u-channel debug image to '%s'", image->channels, filename->str);
	writeImageToFile(image, filename->str);
}

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
