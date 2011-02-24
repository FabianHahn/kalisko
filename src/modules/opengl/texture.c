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
 * @result				the created texture
 */
API OpenGLTexture *createOpenGLTexture(Image *image)
{
	OpenGLTexture *texture = ALLOCATE_OBJECT(OpenGLTexture);
	texture->image = image;

	// Create texture
	glGenTextures(1, &texture->texture);
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // regenerate mipmaps on update
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // use mipmaps to interpolate
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	updateOpenGLTexture(texture);

	if(checkOpenGLError()) {
		freeOpenGLTexture(texture);
		return NULL;
	}

	return texture;
}

/**
 * Updates an OpenGL texture by synchronizing it's CPU-side buffer with the OpenGL texture context
 *
 * @param texture		the texture to update
 * @result				true if successful
 */
API bool updateOpenGLTexture(OpenGLTexture *texture)
{
	glBindTexture(GL_TEXTURE_2D, texture->texture);

	GLenum format;
	switch(texture->image->channels) {
		case 1:
			format = GL_LUMINANCE;
		break;
		case 2:
			format = GL_LUMINANCE_ALPHA;
		break;
		case 3:
			format = GL_RGB;
		break;
		case 4:
			format = GL_RGBA;
		break;
		default:
			LOG_ERROR("Failed to update OpenGL texture: Unsupported number of image channels - %d", texture->image->channels);
			return false;
		break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, texture->image->channels, texture->image->width, texture->image->height, 0, format, GL_UNSIGNED_BYTE, texture->image->data);


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
