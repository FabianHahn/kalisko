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
MODULE_VERSION(0, 5, 8);
MODULE_BCVERSION(0, 5, 0);
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

	// shift the whole image
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			setImage(image, x, y, channel, minValue + (maxValue - minValue) * getImageFloat(image, x, y, channel));
		}
	}
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
