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

#ifndef OPENGL_UNIFORM_H
#define OPENGL_UNIFORM_H

#include <GL/glew.h>
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"
#include "texture.h"

/**
 * Enum for possible OpenGL uniform types
 */
typedef enum {
	/** A uniform int value */
	OPENGL_UNIFORM_INT,
	/** A uniform int pointer value */
	OPENGL_UNIFORM_INT_POINTER,
	/** A uniform float value */
	OPENGL_UNIFORM_FLOAT,
	/** A uniform float pointer value */
	OPENGL_UNIFORM_FLOAT_POINTER,
	/** A uniform 4-vector value */
	OPENGL_UNIFORM_VECTOR,
	/** A uniform 4x4-matrix value */
	OPENGL_UNIFORM_MATRIX,
	/** A texture value */
	OPENGL_UNIFORM_TEXTURE
} OpenGLUniformType;

/**
 * Union for possible OpenGL uniform values
 */
typedef union {
	/** A uniform int value */
	int int_value;
	/** A uniform int pointer value */
	int *int_pointer_value;
	/** A uniform float value */
	float float_value;
	/** A uniform float pointer value */
	float *float_pointer_value;
	/** A uniform vector value */
	Vector *vector_value;
	/** A uniform matrix value */
	Matrix *matrix_value;
	/** A texture value */
	OpenGLTexture *texture_value;
} OpenGLUniformContent;

/**
 * Struct representing an OpenGL uniform value
 */
typedef struct {
	/** The type of the uniform value */
	OpenGLUniformType type;
	/** The content of the uniform value */
	OpenGLUniformContent content;
	/** The location of the uniform in the shader program */
	GLint location;
} OpenGLUniform;

/**
 * Struct representing an OpenGL uniform attachment point
 */
typedef struct {
	/** The uniforms attached to this attachment point */
	GHashTable *uniforms;
	/** Specifies whether the uniform locations are static and can be cached */
	bool staticLocation;
} OpenGLUniformAttachment;

API void initOpenGLUniforms();
API void freeOpenGLUniforms();
API OpenGLUniformAttachment *getOpenGLGlobalUniforms();
API OpenGLUniform *createOpenGLUniformInt(int value);
API OpenGLUniform *createOpenGLUniformIntPointer(int *value);
API OpenGLUniform *createOpenGLUniformFloat(double value);
API OpenGLUniform *createOpenGLUniformFloatPointer(float *value);
API OpenGLUniform *createOpenGLUniformVector(Vector *value);
API OpenGLUniform *createOpenGLUniformMatrix(Matrix *value);
API OpenGLUniform *createOpenGLUniformTexture(OpenGLTexture *texture);
API OpenGLUniform *copyOpenGLUniform(OpenGLUniform *uniform);
API bool useOpenGLUniform(OpenGLUniform *uniform);
API OpenGLUniformAttachment *createOpenGLUniformAttachment();
API bool attachOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name, OpenGLUniform *uniform);
API OpenGLUniform *getOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name);
API bool detachOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name);
API bool useOpenGLUniformAttachment(OpenGLUniformAttachment *attachment, GLuint program, unsigned int *textureIndex);
API void freeOpenGLUniformAttachment(OpenGLUniformAttachment *attachment);

#endif