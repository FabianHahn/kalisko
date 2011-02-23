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

#ifndef IMAGE_IMAGE_H
#define IMAGE_IMAGE_H

#include <assert.h>

/**
 * Struct representing an image
 */
typedef struct {
	/** the width of the image */
	unsigned int width;
	/** the height of the image */
	unsigned int height;
	/** the number of image channels */
	unsigned int channels;
	/** the image data */
	float *data;
} Image;

API Image *createImage(unsigned int width, unsigned int height, unsigned int channels);
API void freeImage(Image *image);

/**
 * Retrieves an image pixel
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
inline float getImage(Image *image, unsigned int x, unsigned int y, unsigned int c)
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
inline float setImage(Image *image, unsigned int x, unsigned int y, unsigned int c, float value)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	image->data[y * image->width * image->channels + x * image->channels + c] = value;
}

#endif
