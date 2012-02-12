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

#include <png.h>
#include <stdio.h>
#include <setjmp.h>
#include "dll.h"
#include "modules/image/io.h"
#define API

MODULE_NAME("image_png");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing support for the PNG image data type");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 16));

static Image *readImageFilePng(const char *filename);
static bool writeImageFilePng(const char *filename, Image *image);

MODULE_INIT
{
	if(!addImageIOReadHandler("png", &readImageFilePng)) {
		return false;
	}

	if(!addImageIOWriteHandler("png", &writeImageFilePng)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	deleteImageIOReadHandler("png");
	deleteImageIOWriteHandler("png");
}

/**
 * Reads an image from a png file
 *
 * @param filename			the png file to read from
 * @result					the parsed image or NULL on failure
 */
static Image *readImageFilePng(const char *filename)
{
	png_byte header[8]; // 8 is the maximum size that can be checked

	FILE *file;

	if((file = fopen(filename, "rb")) == NULL) {
		LOG_SYSTEM_ERROR("Could not open image file %s", filename);
		return NULL;
	}

	if(fread(header, 1, 8, file) <= 0 || png_sig_cmp(header, 0, 8)) {
		LOG_ERROR("Failed to read PNG image '%s': libpng header signature mismatch", filename);
		fclose(file);
		return NULL;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_infop end_info = png_create_info_struct(png_ptr);

	unsigned int height = 0;
	png_bytep *row_pointers = NULL;

	if(setjmp(png_jmpbuf(png_ptr))) {
		LOG_ERROR("Failed to read PNG image '%s': libpng called longjmp", filename);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(file);

		// Cleanup memory if already allocated
		if(row_pointers != NULL) {
			for(unsigned int y = 0; y < height; y++) {
				free(row_pointers[y]);
			}
			free(row_pointers);
		}

		return NULL;
	}

	png_init_io(png_ptr, file);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);
	unsigned int width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if(bit_depth == 16) { // make sure values fit into a char
		png_set_strip_16(png_ptr);
	}

	png_set_packing(png_ptr);

	unsigned int channels = 0;

	switch(color_type) {
		case PNG_COLOR_TYPE_GRAY:
			channels = 1;
		break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			channels = 2;
		break;
		case PNG_COLOR_TYPE_PALETTE:
			channels = 3;
			png_set_palette_to_rgb(png_ptr);
		break;
		case PNG_COLOR_TYPE_RGB:
			channels = 3;
		break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			channels = 4;
		break;
		default:
			LOG_ERROR("Read PNG image '%s' has unsupported color type", filename);
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(file);
			return NULL;
		break;
	}

	row_pointers = ALLOCATE_OBJECTS(png_bytep, height);
	for(unsigned int y = 0; y < height; y++) {
		row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));
	}

	png_read_image(png_ptr, row_pointers);

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	fclose(file);

	LOG_DEBUG("Read PNG image '%s' has dimension %ux%u, bit depth %d and %u channels", filename, width, height, bit_depth, channels);

	Image *image = $(Image *, image, createImageByte)(width, height, channels);
	for(unsigned int y = 0; y < height; y++) {
		png_byte *row = row_pointers[y];
		for(unsigned int x = 0; x < width; x++) {
			unsigned char *ptr = &(row[x * channels]);
			for(unsigned int c = 0; c < channels; c++) {
				setImageByte(image, x, y, c, ptr[c]);
			}
		}
	}

	// Cleanup
	for(unsigned int y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	return image;
}

/**
 * Writes an image to a png file
 *
 * @param filename			the png file to write to
 * @param image				the image to write
 * @result					true if successful
 */
static bool writeImageFilePng(const char *filename, Image *image)
{
	if(image->channels > 4) {
		LOG_ERROR("Cannot save images with more than 4 channels as PNG");
		return false;
	}

	FILE *file;

	if((file = fopen(filename, "wb")) == NULL) {
		LOG_SYSTEM_ERROR("Could not open image file %s", filename);
		return false;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(png_ptr == NULL) {
		LOG_ERROR("Failed to create libpng write struct");
		fclose(file);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		LOG_ERROR("Failed to create libpng info struct");
		fclose(file);
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	png_bytep *row_pointers = ALLOCATE_OBJECTS(png_bytep, image->height);
	for(unsigned int y = 0; y < image->height; y++) {
		row_pointers[y] = ALLOCATE_OBJECTS(png_byte, image->width * image->channels);

		for(unsigned int x = 0; x < image->width; x++) {
			for(unsigned int c = 0; c < image->channels; c++) {
				row_pointers[y][image->channels * x + c] = getImageAsByte(image, x, y, c);
			}
		}
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		LOG_ERROR("Failed to write PNG image '%s': libpng called longjmp", filename);
		fclose(file);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		// Cleanup memory
		for(unsigned int y = 0; y < image->height; y++) {
			free(row_pointers[y]);
		}
		free(row_pointers);

		return false;
	}

	// set up writing
	png_init_io(png_ptr, file);

	png_byte colorType = 0;
	switch(image->channels) {
		case 1:
			colorType = PNG_COLOR_TYPE_GRAY;
		break;
		case 2:
			colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
		case 3:
			colorType = PNG_COLOR_TYPE_RGB;
		break;
		case 4:
			colorType = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	}

	// prepare header
	png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// prepare image contents
	png_set_rows(png_ptr, info_ptr, row_pointers);

	// write it to disk
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	// free the libpng context
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(file);

	// Cleanup memory
	for(unsigned int y = 0; y < image->height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	return true;
}
