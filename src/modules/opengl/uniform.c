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
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "material.h"
#include "texture.h"
#include "uniform.h"

/**
 * Creates an int valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformInt(int value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_INT;
	uniform->content.int_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates an int pointer valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformIntPointer(int *value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_INT_POINTER;
	uniform->content.int_pointer_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a float valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformFloat(double value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_FLOAT;
	uniform->content.float_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a float pointer valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformFloatPointer(float *value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_FLOAT_POINTER;
	uniform->content.float_pointer_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a vector valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be a 4-vector
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformVector(Vector *value)
{
	unsigned int size = $(unsigned int, linalg, getVectorSize)(value);
	if(!(size == 3 || size == 4)) {
		LOG_ERROR("Failed to create vector uniform with size %u instead of 3 or 4", size);
		return NULL;
	}

	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_VECTOR;
	uniform->content.vector_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a matrix valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be a 4x4-matrix
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformMatrix(Matrix *value)
{
	unsigned int rows = $(unsigned int, linalg, getMatrixRows)(value);
	unsigned int cols = $(unsigned int, linalg, getMatrixCols)(value);
	if(rows != 4 || cols != 4) {
		LOG_ERROR("Failed to create matrix uniform with size %ux%u instead of 4x4", rows, cols);
		return NULL;
	}

	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_MATRIX;
	uniform->content.matrix_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a texture valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be an OpenGL texture
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformTexture(OpenGLTexture *texture)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_TEXTURE;
	uniform->content.texture_value = texture;
	uniform->location = -1;

	return uniform;
}

/**
 * Copies an OpenGL uniform
 *
 * @param uniform			the OpenGL uniform to be copied
 * @result					the duplicated uniform
 */
API OpenGLUniform *copyOpenGLUniform(OpenGLUniform *uniform)
{
	OpenGLUniform *newUniform = ALLOCATE_OBJECT(OpenGLUniform);
	newUniform->type = uniform->type;
	newUniform->content = uniform->content;
	newUniform->location = -1;

	return newUniform;
}

/**
 * Uses a uniform in the current shader program
 *
 * @param uniform		the uniform to use
 * @result				true if successful
 */
API bool useOpenGLUniform(OpenGLUniform *uniform)
{
	if(uniform->location == -1) {
		LOG_ERROR("Tried to use uniform with unspecified location, aborting");
		return false;
	}

	switch(uniform->type) {
		case OPENGL_UNIFORM_INT:
			glUniform1i(uniform->location, uniform->content.int_value);
		break;
		case OPENGL_UNIFORM_INT_POINTER:
			glUniform1i(uniform->location, *uniform->content.int_pointer_value);
		break;
		case OPENGL_UNIFORM_FLOAT:
			glUniform1f(uniform->location, uniform->content.float_value);
		break;
		case OPENGL_UNIFORM_FLOAT_POINTER:
			glUniform1f(uniform->location, *uniform->content.float_pointer_value);
		break;
		case OPENGL_UNIFORM_VECTOR:
			if($(unsigned int, linalg, getVectorSize)(uniform->content.vector_value) == 3) {
				glUniform3fv(uniform->location, 1, $(float *, linalg, getVectorData)(uniform->content.vector_value));
			} else {
				glUniform4fv(uniform->location, 1, $(float *, linalg, getVectorData)(uniform->content.vector_value));
			}
		break;
		case OPENGL_UNIFORM_MATRIX:
			glUniformMatrix4fv(uniform->location, 1, GL_TRUE, $(float *, linalg, getMatrixData)(uniform->content.matrix_value));
		break;
		case OPENGL_UNIFORM_TEXTURE:
			glUniform1i(uniform->location, uniform->content.texture_value->unit);
		break;
	}

	return true;
}
