/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *	 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *	 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *	   in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <limits.h>
#include "dll.h"
#include "modules/image/io.h"
#include "api.h"

MODULE_NAME("image_pnm");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing support for the PNM image data types");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 5));

static Image *readImageFilePPM(const char *fileName);
static bool writeImageFilePPM(const char* fileName, Image* image);

MODULE_INIT
{
	if(!$(bool, Image, addImageIOReadHandler)("ppm", &readImageFilePPM)) {
		return false;
	}
	if(!$(bool, Image, addImageIOWriteHandler)("ppm", &writeImageFilePPM)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, const char *, deleteImageIOWriteHandler)("ppm");
}

/**
 * Reads an image from a PPM (portable pixmap format) file
 *
 * Note: Only the ASCII version of PPM format is supported.
 *
 * @param fileName			the ppm file to read from
 * @result					the parsed image or NULL on failure
 */
static Image *readImageFilePPM(const char *fileName)
{
	assert(fileName != NULL);

	FILE* file;
	file = fopen(fileName, "r");

	if(file == NULL) {
		LOG_ERROR("Failed to read PPM image '%s': fopen failed", fileName);
		return NULL;
	}

	// read magic number
	char magic[4];
	fgets(magic, 4, file);
	if(strncmp(magic, "P3", 2) != 0)
	{
		LOG_ERROR("Failed to read PPM image '%s': invalid magic number", fileName);
		fclose(file);
		return NULL;
	}

	// skip comments
	int c = getc(file);
	while(c == '#')
	{
		// go to end of line
		while(c != '\n') {
			if(c == EOF) {
				LOG_ERROR("Failed to read PPM image '%s': invalid header", fileName);
				fclose(file);
				return NULL;
			}
			c = getc(file);
		}
		c = getc(file);
	}
	ungetc(c, file);

	// read header
	unsigned int width = 0, height = 0;
	fscanf(file, "%u %u", &width, &height);

	unsigned int maxValue = 0;
	fscanf(file, "%u", &maxValue);

	if(height == 0 || width == 0 || maxValue == 0) {
		LOG_ERROR("Failed to read PPM image '%s': invalid header", fileName);
		fclose(file);
		return NULL;
	}

	// allocate image
	Image *image = $(Image *, image, createImageByte)(width, height, 3);

	// read pixels
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			unsigned int r, g, b;
			r = g = b = UINT_MAX;
			fscanf(file, "%u", &r);
			fscanf(file, "%u", &g);
			fscanf(file, "%u", &b);
			if(r > maxValue || g > maxValue || b > maxValue) {
				LOG_ERROR("Failed to read PPM image '%s': invalid pixel data", fileName);
				$(void, image, freeImage)(image);
				fclose(file);
				return NULL;
			}
			setImage(image, x, y, 0, r/(double)maxValue);
			setImage(image, x, y, 1, g/(double)maxValue);
			setImage(image, x, y, 2, b/(double)maxValue);
		}
	}

	fclose(file);
	LOG_DEBUG("Read PPN image '%s'", fileName);
	return image;
}

/**
 * Writes an image to a PPM (portable pixmap format) file
 *
 * @param fileName			the ppm file to write to
 * @param image				the image data to write
 * @result					true if successful
 */
static bool writeImageFilePPM(const char* fileName, Image* image)
{
	assert(fileName != NULL);
	assert(image != NULL);

	if(image->channels < 3) {
		LOG_ERROR("Failed to write PPM image '%s': only RGB images are supported", fileName);
		return false;
	}

	FILE* file;
	file = fopen(fileName, "w");

	if(file == NULL) {
		LOG_ERROR("Failed to write PPM image '%s': fopen failed", fileName);
		return false;
	}

	// write the PPM header
	fputs("P3\n", file); // file type
	fputs("# PPM ASCII RGB image created by Kalisko\n", file); // comment
	fprintf(file, "%u %u\n", image->height, image->width); // image size
	fputs("255\n", file); // max. channel value

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			fprintf(file, "%u %u %u\n", getImageByte(image, x, y, 0),
									    getImageByte(image, x, y, 1),
									    getImageByte(image, x, y, 2));
		}
	}

	fclose(file);
	LOG_DEBUG("Wrote PPN image '%s'", fileName);

	return true;
}
