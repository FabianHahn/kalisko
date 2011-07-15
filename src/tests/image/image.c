/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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

#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

#include "dll.h"
#include "test.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#include "api.h"

#ifdef WIN32
#define TMPFILE "kalisko_test_image.store"
#else
#define TMPFILE "/tmp/kalisko_test_image.store"
#endif

TEST_CASE(io);
TEST_CASE(convert);
static Image *createTestImage();

MODULE_NAME("test_image");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the image module");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 16));

TEST_SUITE_BEGIN(image)
	TEST_CASE_ADD(io);
	TEST_CASE_ADD(convert);
TEST_SUITE_END

TEST_CASE(io)
{
	Image *image = createTestImage();
	TEST_ASSERT(image != NULL);

	TEST_ASSERT($(bool, image, writeImageToFile)(image, TMPFILE));

	Image *read;
	TEST_ASSERT((read = $(Image *, image, readImageFromFile)(TMPFILE)) != NULL);

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			for(unsigned int c = 0; c < image->channels; c++) {
				TEST_ASSERT(getImage(image, x, y, c) == getImage(read, x, y, c));
			}
		}
	}

	$(void, image, freeImage)(image);
	$(void, image, freeImage)(read);
	g_unlink(TMPFILE);

	TEST_PASS;
}

TEST_CASE(convert)
{
	Image *image = createTestImage();
	TEST_ASSERT(image != NULL);

	Image *copy;
	TEST_ASSERT((copy = $(Image *, image, copyImage)(image, IMAGE_TYPE_FLOAT)) != NULL);
	TEST_ASSERT(copy->type == IMAGE_TYPE_FLOAT);

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			for(unsigned int c = 0; c < image->channels; c++) {
				TEST_ASSERT(getImage(image, x, y, c) == getImage(copy, x, y, c));
			}
		}
	}

	Image *copy2;
	TEST_ASSERT((copy2 = $(Image *, image, copyImage)(copy, IMAGE_TYPE_BYTE)) != NULL);
	TEST_ASSERT(copy2->type == IMAGE_TYPE_BYTE);

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			for(unsigned int c = 0; c < image->channels; c++) {
				TEST_ASSERT(getImage(image, x, y, c) == getImage(copy2, x, y, c));
			}
		}
	}


	$(void, image, freeImage)(image);
	$(void, image, freeImage)(copy);
	$(void, image, freeImage)(copy2);

	TEST_PASS;
}

static Image *createTestImage()
{
	Image *image = $(Image *, image, createImageByte)(10, 10, 3);

	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			for(unsigned int c = 0; c < image->channels; c++) {
				setImageByte(image, x, y, c, x + y + c);
			}
		}
	}

	return image;
}
