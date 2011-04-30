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
 * Creates an OpenGL texture from an image
 *
 * @param image			the image from which to create the texture
 * @param auto_init		if true, initializes the texture with default parameters and synchronizes it (i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLTexture(Image *image, bool auto_init)
{
	OpenGLTexture *texture = ALLOCATE_OBJECT(OpenGLTexture);
	texture->image = image;
	texture->format = -1;
	texture->internalFormat = -1;

	// Create texture
	glGenTextures(1, &texture->texture);
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	if(auto_init) {
		// Set texture parameters
		texture->samplingMode = OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR;

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
 * Creates an OpenGL vertex texture from an image to be used in a vertex shader. The texture does not use mipmaps and is automatically initialized, i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it
 *
 * @param image			the image from which to create the texture
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLVertexTexture(Image *image)
{
	OpenGLTexture *texture = createOpenGLTexture(image, false);

	if(texture == NULL) {
		return NULL;
	}

	texture->samplingMode = OPENGL_TEXTURE_SAMPLING_NEAREST;

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
 * Initializes an OpenGL texture
 *
 * @param texture			the OpenGL texture to initialize
 * @result					true if successful
 */
API bool initOpenGLTexture(OpenGLTexture *texture)
{
	glBindTexture(GL_TEXTURE_2D, texture->texture);

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
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_MIPMAP_NEAREST:
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // regenerate mipmaps on update
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); // use mipmaps to interpolate
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		break;
		case OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // regenerate mipmaps on update
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // use mipmaps to interpolate
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->image->width, texture->image->height, 0, texture->format, GL_UNSIGNED_BYTE, texture->image->data);


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
