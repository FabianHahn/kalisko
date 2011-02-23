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
#include "dll.h"
#include "api.h"
#include "io.h"
#include "image.h"

MODULE_NAME("image");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a general image data type");
MODULE_VERSION(0, 4, 0);
MODULE_BCVERSION(0, 4, 0);
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
 * Creates a new image
 *
 * @param width			the width of the image to create
 * @param height		the height of the image to create
 * @param channels		the number of image channels to create
 * @result				the created image
 */
API Image *createImage(unsigned int width, unsigned int height, unsigned int channels)
{
	Image *image = ALLOCATE_OBJECT(Image);
	image->width = width;
	image->height = height;
	image->channels = channels;
	image->data = ALLOCATE_OBJECTS(unsigned char, width * height * channels);

	return image;
}

/**
 * Retrieves an image pixel
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
API unsigned char getImage(Image *image, unsigned int x, unsigned int y, unsigned int c)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	return image->data[y * image->width * image->channels + x * image->channels + c];
}

/**
 * Sets an image pixel
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @param value			the value to set at the specified location
 */
API void setImage(Image *image, unsigned int x, unsigned int y, unsigned int c, unsigned char value)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	image->data[y * image->width * image->channels + x * image->channels + c] = value;
}

/**
 * Frees an image
 *
 * @param image			the image to free
 */
API void freeImage(Image *image)
{
	free(image->data);
	free(image);
}
