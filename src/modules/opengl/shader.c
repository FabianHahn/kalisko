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
