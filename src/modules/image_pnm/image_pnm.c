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
#include "dll.h"
#include "modules/image/io.h"
#include "api.h"

MODULE_NAME("image_pnm");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing support for the PNM image data types");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 5));

static bool writeImageFilePPM(const char* fileName, Image* image);

MODULE_INIT
{
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
 * Writes an image to a PPM (portable pixmap format) file
 *
 * @param fileName			the ppm file to write to
 * @param image				the image data to write
 * @result					true if successful
 */
static bool writeImageFilePPM(const char* fileName, Image* image)
{
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
	fputs("# PPM ASCII RGB file\n", file); // comment
	fprintf(file, "%d %d\n", image->height, image->width); // image size
	fputs("255\n", file); // max. value

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			fprintf(file, "%u %u %u ", getImageByte(image, x, y, 0),
									   getImageByte(image, x, y, 1),
									   getImageByte(image, x, y, 2));
		}
		fputs("\n", file);
	}

	fclose(file);
	LOG_DEBUG("Wrote PPN image '%s'", fileName);

	return true;
}
