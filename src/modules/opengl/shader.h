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

#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"

/**
 * Enum for possible OpenGL uniform types
 */
typedef enum {
	/** A uniform int value */
	OPENGL_UNIFORM_INT,
	/** A uniform float value */
	OPENGL_UNIFORM_FLOAT,
	/** A uniform 4-vector value */
	OPENGL_UNIFORM_VECTOR,
	/** A uniform 4x4-matrix value */
	OPENGL_UNIFORM_MATRIX
} OpenGLUniformType;

/**
 * Enum for possible predefined OpenGL vertex shader attribute locations
 */
typedef enum {
	/** The vertex attribute */
	OPENGL_ATTRIBUTE_VERTEX,
	/** The normal attribute */
	OPENGL_ATTRIBUTE_NORMAL,
	/** The color attribute */
	OPENGL_ATTRIBUTE_COLOR
} OpenGLAttributeLocation;

/**
 * Union for possible OpenGL uniform values
 */
typedef union {
	/** A uniform int value */
	int int_value;
	/** A uniform float value */
	float float_value;
	/** A uniform vector value */
	Vector *vector_value;
	/** A uniform matrix value */
	Matrix *matrix_value;
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

API GLuint createShaderFromString(const char *source, GLenum type);
API GLuint createShaderProgram(GLuint vertexShader, GLuint fragmentShader, bool recycleShaders);
API OpenGLUniform *createUniformInt(int value);
API OpenGLUniform *createUniformFloat(float value);
API OpenGLUniform *createUniformVector(Vector *value);
API OpenGLUniform *createUniformMatrix(Matrix *value);
API bool useUniform(OpenGLUniform *uniform);

#endif
