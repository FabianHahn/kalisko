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
	/** The OpenGL texture context for this texture */
	GLuint texture;
	/** The OpenGL texture unit currently used to render this texture */
	int unit;
	/** The sampling mode to use for this texture */
	OpenGLTextureSamplingMode samplingMode;
	/** The texture format to use for this texture */
	GLuint format;
	/** The internal texture format to use for this texture */
	GLuint internalFormat;
} OpenGLTexture;

API OpenGLTexture *createOpenGLTexture2D(Image *image, bool auto_init);
API OpenGLTexture *createOpenGLVertexTexture2D(Image *image);
API bool initOpenGLTexture(OpenGLTexture *texture);
API bool synchronizeOpenGLTexture(OpenGLTexture *texture);
API void freeOpenGLTexture(OpenGLTexture *texture);

#endif
