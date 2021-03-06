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

#ifndef OPENGL_TEXTURE_H
#define OPENGL_TEXTURE_H

#include <GL/glew.h>
#include "modules/image/image.h"

/**
 * Specifies the mipmap mode to use for an OpenGL texture
 */
typedef enum {
	/** No mipmaps should be generated, use nearest neighbor lookup */
	OPENGL_TEXTURE_SAMPLING_NEAREST,
	/** No mipmaps should be generated, use linear lookup */
	OPENGL_TEXTURE_SAMPLING_LINEAR,
	/** Generate mipmaps and use nearest neighbour lookup */
	OPENGL_TEXTURE_SAMPLING_MIPMAP_NEAREST,
	/** Generate mipmaps and use linear interpolation lookup */
	OPENGL_TEXTURE_SAMPLING_MIPMAP_LINEAR
} OpenGLTextureSamplingMode;

/**
 * Specifies the wrapping mode to use for an OpenGL texture
 */
typedef enum {
	/** The texture values are simply clamped to the range of the provided image */
	OPENGL_TEXTURE_WRAPPING_CLAMP,
	/** The texture values are repeated modulo the dimension of the provided image */
	OPENGL_TEXTURE_WRAPPING_REPEAT,
	/** The texture values are mirrored at the borders of the image */
	OPENGL_TEXTURE_WRAPPING_MIRROR
} OpenGLTextureWrappingMode;

/**
 * Specifies the type of an OpenGL texture
 */
typedef enum {
	/** A 2D texture */
	OPENGL_TEXTURE_TYPE_2D,
	/** An array of 2D textures */
	OPENGL_TEXTURE_TYPE_2D_ARRAY
} OpenGLTextureType;

/**
 * Struct representing an OpenGL texture that can be attached to shaders as uniform
 */
typedef struct {
	/** The image data of the texture */
	Image *image;
	/** The texture type of the texture */
	OpenGLTextureType type;
	/** If the texture is an array, specifies its size */
	unsigned int arraySize;
	/** The OpenGL texture context for this texture */
	GLuint texture;
	/** The OpenGL texture unit currently used to render this texture */
	int unit;
	/** The sampling mode to use for this texture */
	OpenGLTextureSamplingMode samplingMode;
	/** The wrapping mode to use for this texture */
	OpenGLTextureWrappingMode wrappingMode;
	/** The texture format to use for this texture */
	GLuint format;
	/** The internal texture format to use for this texture */
	GLuint internalFormat;
	/** Specifies whether the texture image should be freed along with the texture */
	bool managed;
} OpenGLTexture;


/**
 * Creates an OpenGL 2D texture from an image
 *
 * @param image			the image from which to create the texture (note that the texture takes control over the image, i.e. you must not free it)
 * @param auto_init		if true, initializes the texture with default parameters and synchronizes it (i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLTexture2D(Image *image, bool auto_init);

/**
 * Creates an OpenGL 2D vertex texture from an image to be used in a vertex shader. The texture does not use mipmaps and is automatically initialized, i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it
 *
 * @param image			the image from which to create the texture (note that the texture takes control over the image, i.e. you must not free it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLVertexTexture2D(Image *image);

/**
 * Creates an OpenGL 2D texture array from an array of images
 *
 * @param images		an array of images from which to create the texture array (note that the texture does NOT take over control over this array or any of its elements, i.e. you must free it yourself after calling this function)
 * @param size			the size of the passed images array
 * @param auto_init		if true, initializes the texture array with default parameters and synchronizes it (i.e. you don't have to call initOpenGLTexture or synchronizeOpenGLTexture before using it)
 * @result				the created texture or NULL on failure
 */
API OpenGLTexture *createOpenGLTexture2DArray(Image **images, unsigned int size, bool auto_init);

/**
 * Initializes an OpenGL texture
 *
 * @param texture			the OpenGL texture to initialize
 * @result					true if successful
 */
API bool initOpenGLTexture(OpenGLTexture *texture);

/**
 * Updates an OpenGL texture by synchronizing it's CPU-side buffer with the OpenGL texture context
 *
 * @param texture		the texture to update
 * @result				true if successful
 */
API bool synchronizeOpenGLTexture(OpenGLTexture *texture);

/**
 * Frees an existing OpenGL texture including the CPU-side buffer
 *
 * @param texture		the texture to free
 */
API void freeOpenGLTexture(OpenGLTexture *texture);

/**
 * Binds an OpenGL texture
 *
 * @param texture			the texture to bind
 */
static inline void bindOpenGLTexture(OpenGLTexture *texture)
{
	switch(texture->type) {
		case OPENGL_TEXTURE_TYPE_2D:
			glBindTexture(GL_TEXTURE_2D, texture->texture);
		break;
		case OPENGL_TEXTURE_TYPE_2D_ARRAY:
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture->texture);
		break;
		default:
			logError("Failed to bind OpenGL texture: Unsupported texture type '%d'", texture->type);
		break;
	}
}

#endif
