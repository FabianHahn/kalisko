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

#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "api.h"
#include "material.h"
#include "shader.h"
#include "uniform.h"

/**
 * Creates an OpenGL shader from a string
 *
 * @param source	the source code of the shader
 * @param type		the type of the shader, usually either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @result			the compiled shader identifier, or 0 on error
 */
API GLuint createOpenGLShaderFromString(const char *source, GLenum type)
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
 * Creates an OpenGL shader from a file
 *
 * @param filename	the file name of the shader
 * @param type		the type of the shader, usually either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @result			the compiled shader identifier, or 0 on error
 */
API GLuint createOpenGLShaderFromFile(const char *filename, GLenum type)
{
	char *shaderSource;
	gsize shaderLength;
	if(!g_file_get_contents(filename, &shaderSource, &shaderLength, NULL)) {
		LOG_ERROR("Failed to read shader source from %s", filename);
		return false;
	}

	// Create the shader from the read source string
	GLuint shader = createOpenGLShaderFromString(shaderSource, type);

	// Free read file
	free(shaderSource);

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
API GLuint createOpenGLShaderProgram(GLuint vertexShader, GLuint fragmentShader, bool recycleShaders)
{
	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glBindAttribLocation(program, OPENGL_ATTRIBUTE_POSITION, "position");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_NORMAL, "normal");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_COLOR, "color");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_UV, "uv");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_BIRTH, "birth");
	glBindAttribLocation(program, OPENGL_ATTRIBUTE_ANGULAR_VELOCITY, "angularVelocity");

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
