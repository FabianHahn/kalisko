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
 * Enum type describing possible image types
 */
typedef enum {
	/** An image containing byte channels */
	IMAGE_TYPE_BYTE,
	/** An image containing float channels */
	IMAGE_TYPE_FLOAT
} ImageType;

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
	/** the type of the image */
	ImageType type;
	/** the image data */
	union {
		/** byte data if the type is IMAGE_TYPE_BYTE */
		unsigned char *byte_data;
		/** float data if the type is IMAGE_TYPE_FLOAT */
		float *float_data;
	} data;
} Image;

API Image *createImageByte(unsigned int width, unsigned int height, unsigned int channels);
API Image *createImageFloat(unsigned int width, unsigned int height, unsigned int channels);
API Image *createImage(unsigned int width, unsigned int height, unsigned int channels, ImageType type);
API Image *copyImage(Image *source, ImageType targetType);
API void clearImage(Image *image);
API void clearImageChannel(Image *image, unsigned int channel);
API void normalizeImageChannel(Image *image, unsigned int channel);
API void invertImageChannel(Image *image, unsigned int channel);
API void scaleImageChannel(Image *image, unsigned int channel, float factor);
API Image *blendImages(Image *a, Image *b, double factor);
API void freeImage(Image *image);

/**
 * Retrieves an image pixel from a byte image
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
static inline unsigned char getImageByte(Image *image, unsigned int x, unsigned int y, unsigned int c)
{
	assert(image->type == IMAGE_TYPE_BYTE);
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	return image->data.byte_data[y * image->width * image->channels + x * image->channels + c];
}

/**
 * Retrieves an image pixel from a float image
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
static inline float getImageFloat(Image *image, unsigned int x, unsigned int y, unsigned int c)
{
	assert(image->type == IMAGE_TYPE_FLOAT);
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	return image->data.float_data[y * image->width * image->channels + x * image->channels + c];
}

/**
 * Retrieves an image pixel from an image as a byte value
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
static inline unsigned int getImageAsByte(Image *image, unsigned int x, unsigned int y, unsigned int c)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	if(image->type == IMAGE_TYPE_BYTE) {
		return image->data.byte_data[y * image->width * image->channels + x * image->channels + c];
	} else if(image->type == IMAGE_TYPE_FLOAT) {
		int scaled = 255.0f * image->data.float_data[y * image->width * image->channels + x * image->channels + c];
		if(scaled > 255) {
			scaled = 255;
		}

		if(scaled < 0) {
			scaled = 0;
		}

		return scaled;
	}

	return 0; // never reached
}

/**
 * Retrieves an image pixel from an image
 *
 * @param image			the image from which to retrieve the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @result				the pixel at the requested position
 */
static inline float getImage(Image *image, unsigned int x, unsigned int y, unsigned int c)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			return image->data.byte_data[y * image->width * image->channels + x * image->channels + c] / 255.0f;
		break;
		case IMAGE_TYPE_FLOAT:
			return image->data.float_data[y * image->width * image->channels + x * image->channels + c];
		break;
	}

	return 0.0f; // never reached
}

/**
 * Sets an image pixel for a byte image
 *
 * @param image			the image from which to set the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @param value			the value to set at the specified location
 */
static inline void setImageByte(Image *image, unsigned int x, unsigned int y, unsigned int c, unsigned char value)
{
	assert(image->type == IMAGE_TYPE_BYTE);
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	image->data.byte_data[y * image->width * image->channels + x * image->channels + c] = value;
}

/**
 * Sets an image pixel for a byte image
 *
 * @param image			the image from which to set the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @param value			the value to set at the specified location
 */
static inline void setImageFloat(Image *image, unsigned int x, unsigned int y, unsigned int c, double value)
{
	assert(image->type == IMAGE_TYPE_FLOAT);
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	image->data.float_data[y * image->width * image->channels + x * image->channels + c] = value;
}

/**
 * Sets an image pixel for an image
 *
 * @param image			the image from which to set the pixel
 * @param x				the x location to access
 * @param y				the y location to access
 * @param c				the channel to access
 * @param value			the value to set at the specified location
 */
static inline void setImage(Image *image, unsigned int x, unsigned int y, unsigned int c, double value)
{
	assert(x < image->width);
	assert(y < image->height);
	assert(c < image->channels);

	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			image->data.byte_data[y * image->width * image->channels + x * image->channels + c] = 255 * value;
		break;
		case IMAGE_TYPE_FLOAT:
			image->data.float_data[y * image->width * image->channels + x * image->channels + c] = value;
		break;
	}
}

/**
 * Returns the size of one image pixel in an image
 *
 * @param image			the image for which to lookup the size
 * @result				the looked up image pixel size
 */
static inline unsigned int getImagePixelSize(Image *image)
{
	unsigned int pixelSize = 0;

	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			pixelSize = sizeof(unsigned char);
		break;
		case IMAGE_TYPE_FLOAT:
			pixelSize = sizeof(float);
		break;
	}

	return pixelSize;
}

/**
 * Returns a pointer to the raw data of an image
 *
 * @param image			the image for which to lookup the raw data pointer
 * @result				a pointer to the raw data of the image
 */
static inline void *getImageData(Image *image)
{
	void *data = NULL;

	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			data = image->data.byte_data;
		break;
		case IMAGE_TYPE_FLOAT:
			data = image->data.float_data;
		break;
	}

	return data;
}

#endif
