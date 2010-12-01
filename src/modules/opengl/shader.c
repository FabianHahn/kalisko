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

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "dll.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"
#include "memory_alloc.h"
#include "log.h"
#include "api.h"
#include "shader.h"

/**
 * Creates an OpenGL shader from a string
 *
 * @param source	the source code of the shader
 * @param type		the type of the shader, usually either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @result			the compiled shader identifier, or 0 on error
 */
API GLuint createShaderFromString(const char *source, GLenum type)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if(status == 0)
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char errorstr[length];
		glGetShaderInfoLog(shader, length, NULL, errorstr);
		LOG_ERROR("Failed to compile shader: %s", errorstr);
		return 0;
	}

	return shader;
}

/**
 * Creates an OpenGL shader program from a compiled vertex shader and a compiled fragment shader
 *
 * @param vertexShader		the vertex shader to link into the program
 * @param fragmentShader	the fragment shader to link into the program
 * @param recycleShaders	should the shaders be marked for deletion after linking them into the program?
 * @result					the linked shader program, or 0 on error
 */
API GLuint createShaderProgram(GLuint vertexShader, GLuint fragmentShader, bool recycleShaders)
{
	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glBindAttribLocation(program, OPENGL_ATTRIBUTE_VERTEX, "vertex");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_NORMAL, "normal");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_COLOR, "color");

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if(status == 0)
	{
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char errorstr[length];
		glGetProgramInfoLog(program, length, NULL, errorstr);
		LOG_ERROR("Failed to link shader: %s", errorstr);
		return 0;
	}

	if(recycleShaders) {
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	return program;
}

/**
 * Creates an int valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createUniformInt(int value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_INT;
	uniform->content.int_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a float valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createUniformFloat(float value)
{
	OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
	uniform->type = OPENGL_UNIFORM_FLOAT;
	uniform->content.float_value = value;
	uniform->location = -1;

	return uniform;
}

/**
 * Creates a vector valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be a 4-vector
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createUniformVector(Vector *value)
{
	unsigned int size;
	if((size = $(unsigned int, linalg, getVectorSize)(value)) != 4) {
		LOG_ERROR("Failed to create vector uniform with size %u instead of 4", size);
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
API OpenGLUniform *createUniformMatrix(Matrix *value)
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
 * Uses a uniform in the current shader program
 *
 * @param uniform		the uniform to use
 * @result				true if successful
 */
API bool useUniform(OpenGLUniform *uniform)
{
	if(uniform->location == -1) {
		LOG_ERROR("Tried to use uniform with unspecified location, aborting");
		return false;
	}

	switch(uniform->type) {
		case OPENGL_UNIFORM_INT:
			glUniform1i(uniform->location, uniform->content.int_value);
		break;
		case OPENGL_UNIFORM_FLOAT:
			glUniform1f(uniform->location, uniform->content.float_value);
		break;
		case OPENGL_UNIFORM_VECTOR:
			glUniform4fv(uniform->location, 1, $(float *, linalg, getVectorData)(uniform->content.vector_value));
		break;
		case OPENGL_UNIFORM_MATRIX:
			glUniformMatrix4fv(uniform->location, 1, GL_TRUE, $(float *, linalg, getMatrixData)(uniform->content.matrix_value));
		break;
	}

	return true;
}
