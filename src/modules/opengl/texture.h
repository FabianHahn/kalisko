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
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"

/**
 * Struct representing an OpenGL texture that can be attached to shaders as uniform
 */
typedef struct {
	/** The width of the texture */
	unsigned int width;
	/** The height of the texture */
	unsigned int height;
	/** The texture data stored from bottom left pixel to top right pixel */
	float *data;
	/** The OpenGL texture context for this texture */
	GLuint texture;
	/** The OpenGL texture unit currently used to render this texture */
	int unit;
} OpenGLTexture;

API OpenGLTexture *createOpenGLTexture(unsigned int width, unsigned int height);
API bool updateOpenGLTexture(OpenGLTexture *texture);
API void freeOpenGLTexture(OpenGLTexture *texture);

#endif
