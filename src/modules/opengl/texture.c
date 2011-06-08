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

#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/image/image.h"
#include "memory_alloc.h"
#include "api.h"
#include "opengl.h"
#include "texture.h"

/**
 * Creates an OpenGL 2D texture from an image
 *
 * @param image			the image from which to create the texture (note that the texture takes control over the image, i.e. you must not free it)
 * @param auto_init		if true, initializes the texture with default parameters and synchronizes it (i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLTexture2D(Image *image, bool auto_init)
{
	if(image->type != IMAGE_TYPE_BYTE && image->type != IMAGE_TYPE_FLOAT) {
		LOG_ERROR("Failed to create OpenGL 2D texture: Unsupported image type '%d'", image->type);
		return NULL;
	}

	OpenGLTexture *texture = ALLOCATE_OBJECT(OpenGLTexture);
	texture->image = image;
	texture->type = OPENGL_TEXTURE_TYPE_2D;
	texture->arraySize = 0;
	texture->format = -1;
	texture->internalFormat = -1;
	texture->samplingMode = OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR;
	texture->wrappingMode = OPENGL_TEXTURE_WRAPPING_REPEAT;

	// Create texture
	glGenTextures(1, &texture->texture);
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	if(auto_init) {
		if(!initOpenGLTexture(texture)) {
			freeOpenGLTexture(texture);
			return NULL;
		}

		if(!synchronizeOpenGLTexture(texture)) {
			freeOpenGLTexture(texture);
			return NULL;
		}
	}

	if(checkOpenGLError()) {
		freeOpenGLTexture(texture);
		return NULL;
	}

	return texture;
}

/**
 * Creates an OpenGL 2D vertex texture from an image to be used in a vertex shader. The texture does not use mipmaps and is automatically initialized, i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it
 *
 * @param image			the image from which to create the texture (note that the texture takes control over the image, i.e. you must not free it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLVertexTexture2D(Image *image)
{
	OpenGLTexture *texture = createOpenGLTexture2D(image, false);

	if(texture == NULL) {
		return NULL;
	}

	texture->samplingMode = OPENGL_TEXTURE_SAMPLING_NEAREST;
	texture->wrappingMode = OPENGL_TEXTURE_WRAPPING_CLAMP;

	switch(image->channels) {
		case 1:
			texture->internalFormat = GL_LUMINANCE32F_ARB;
		break;
		default:
			texture->internalFormat = GL_RGBA32F;
		break;
	}

	if(!initOpenGLTexture(texture)) {
		freeOpenGLTexture(texture);
		return NULL;
	}

	if(!synchronizeOpenGLTexture(texture)) {
		freeOpenGLTexture(texture);
		return NULL;
	}

	return texture;
}

/**
 * Creates an OpenGL 2D texture array from an array of images
 *
 * @param images		an array of images from which to create the texture array (note that the texture does NOT take over control over this array or any of its elements, i.e. you must free it yourself after calling this function)
 * @param size			the size of the passed images array
 * @param auto_init		if true, initializes the texture array with default parameters and synchronizes it (i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLTexture2DArray(Image **images, unsigned int size, bool auto_init)
{
	if(size == 0) {
		LOG_ERROR("Failed to create OpenGL 2D texture array: Passed empty set of images");
		return NULL;
	}

	// Create large image container for array
	Image *image;
	switch(images[0]->type) {
		case IMAGE_TYPE_BYTE:
			image = $(Image *, image, createImageByte)(images[0]->width, size * images[0]->height, images[0]->channels);
		break;
		case IMAGE_TYPE_FLOAT:
			image = $(Image *, image, createImageFloat)(images[0]->width, size * images[0]->height, images[0]->channels);
		break;
		default:
			LOG_ERROR("Failed to create OpenGL 2D texture array: Unsupported image type '%d'", images[0]->type);
			return NULL;
		break;
	}

	// Copy over the images
	for(unsigned int i = 0; i < size; i++) {
		if(images[i]->type != images[0]->type || images[i]->width != images[0]->width || images[i]->height != images[0]->height || images[i]->channels != images[0]->channels) {
			LOG_ERROR("Failed to create OpenGL 2D texture array: Image '%u' doesn't match the parameters of the first image", i);
			$(void, image, freeImage)(image);
			return NULL;
		}

		for(unsigned int y = 0; y < images[0]->height; y++) {
			for(unsigned int x = 0; x < images[0]->width; x++) {
				for(unsigned int c = 0; c < images[0]->channels; c++) {
					if(images[0]->type == IMAGE_TYPE_BYTE) {
						setImageByte(image, x, i * images[0]->height + y, c, getImageByte(images[i], x, y, c));
					} else {
						setImageFloat(image, x, i * images[0]->height + y, c, getImageFloat(images[i], x, y, c));
					}
				}
			}
		}
	}

	OpenGLTexture *texture = ALLOCATE_OBJECT(OpenGLTexture);
	texture->image = image;
	texture->type = OPENGL_TEXTURE_TYPE_2D_ARRAY;
	texture->arraySize = size;
	texture->format = -1;
	texture->internalFormat = -1;
	texture->samplingMode = OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR;
	texture->wrappingMode = OPENGL_TEXTURE_WRAPPING_REPEAT;

	// Create texture
	glGenTextures(1, &texture->texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->texture);

	if(auto_init) {
		if(!initOpenGLTexture(texture)) {
			freeOpenGLTexture(texture);
			return NULL;
		}

		if(!synchronizeOpenGLTexture(texture)) {
			freeOpenGLTexture(texture);
			return NULL;
		}
	}

	if(checkOpenGLError()) {
		freeOpenGLTexture(texture);
		return NULL;
	}

	return texture;
}

/**
 * Initializes an OpenGL texture
 *
 * @param texture			the OpenGL texture to initialize
 * @result					true if successful
 */
API bool initOpenGLTexture(OpenGLTexture *texture)
{
	bindOpenGLTexture(texture);

	GLuint typeEnum;
	switch(texture->type) {
		case OPENGL_TEXTURE_TYPE_2D:
			typeEnum = GL_TEXTURE_2D;
		break;
		case OPENGL_TEXTURE_TYPE_2D_ARRAY:
			typeEnum = GL_TEXTURE_2D_ARRAY;
		break;
		default:
			LOG_ERROR("Failed to initialize OpenGL texture: Unsupported texture type '%d'", texture->type);
			return false;
		break;
	}

	if(texture->format == -1) { // if the caller didn't specify the format himself, auto-select it
		switch(texture->image->channels) {
			case 1:
				texture->format = GL_LUMINANCE;
			break;
			case 2:
				texture->format = GL_LUMINANCE_ALPHA;
			break;
			case 3:
				texture->format = GL_RGB;
			break;
			case 4:
				texture->format = GL_RGBA;
			break;
			default:
				LOG_ERROR("Failed to init OpenGL texture: Unsupported number of image channels - %d", texture->image->channels);
				return false;
			break;
		}
	}

	if(texture->internalFormat == -1) { // if the caller didn't specify the internalFormat himself, auto-select it
		texture->internalFormat = texture->image->channels;
	}

	switch(texture->samplingMode) {
		case OPENGL_TEXTURE_SAMPLING_NEAREST:
			glTexParameteri(typeEnum, GL_GENERATE_MIPMAP, GL_FALSE);
			glTexParameteri(typeEnum, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(typeEnum, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_LINEAR:
			glTexParameteri(typeEnum, GL_GENERATE_MIPMAP, GL_FALSE);
			glTexParameteri(typeEnum, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(typeEnum, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_MIPMAP_NEAREST:
			glTexParameteri(typeEnum, GL_GENERATE_MIPMAP, GL_TRUE); // regenerate mipmaps on update
			glTexParameteri(typeEnum, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); // use mipmaps to interpolate
			glTexParameteri(typeEnum, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR:
			glTexParameteri(typeEnum, GL_GENERATE_MIPMAP, GL_TRUE); // regenerate mipmaps on update
			glTexParameteri(typeEnum, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // use mipmaps to interpolate
			glTexParameteri(typeEnum, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}

	switch(texture->wrappingMode) {
		case OPENGL_TEXTURE_WRAPPING_CLAMP:
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_T, GL_CLAMP);
		break;
		case OPENGL_TEXTURE_WRAPPING_REPEAT:
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
		case OPENGL_TEXTURE_WRAPPING_MIRROR:
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(typeEnum, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		break;
	}

	if(checkOpenGLError()) {
		LOG_ERROR("Failed to initialize OpenGL texture");
		return false;
	}

	return true;
}

/**
 * Updates an OpenGL texture by synchronizing it's CPU-side buffer with the OpenGL texture context
 *
 * @param texture		the texture to update
 * @result				true if successful
 */
API bool synchronizeOpenGLTexture(OpenGLTexture *texture)
{
	bindOpenGLTexture(texture);

	switch(texture->type) {
		case OPENGL_TEXTURE_TYPE_2D:
			switch(texture->image->type) {
				case IMAGE_TYPE_BYTE:
					glTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->image->width, texture->image->height, 0, texture->format, GL_UNSIGNED_BYTE, texture->image->data.byte_data);
				break;
				case IMAGE_TYPE_FLOAT:
					glTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->image->width, texture->image->height, 0, texture->format, GL_FLOAT, texture->image->data.float_data);
				break;
			}
		break;
		case OPENGL_TEXTURE_TYPE_2D_ARRAY:
			switch(texture->image->type) {
				case IMAGE_TYPE_BYTE:
					glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, texture->internalFormat, texture->image->width, texture->image->height / texture->arraySize, texture->arraySize, 0, texture->format, GL_UNSIGNED_BYTE, texture->image->data.byte_data);
				break;
				case IMAGE_TYPE_FLOAT:
					glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, texture->internalFormat, texture->image->width, texture->image->height / texture->arraySize, texture->arraySize, 0, texture->format, GL_FLOAT, texture->image->data.float_data);
				break;
			}
		break;
		default:
			LOG_ERROR("Failed to initialize OpenGL texture: Unsupported texture type '%d'", texture->type);
			return false;
		break;
	}

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Frees an existing OpenGL texture including the CPU-side buffer
 *
 * @param texture		the texture to free
 */
API void freeOpenGLTexture(OpenGLTexture *texture)
{
	assert(texture != NULL);

	$(void, image, freeImage)(texture->image);
	glDeleteTextures(1, &texture->texture);
	free(texture);
}
