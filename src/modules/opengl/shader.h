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
#include "texture.h"
#include "uniform.h"

/**
 * Enum for possible predefined OpenGL vertex shader attribute locations
 */
typedef enum {
	/** The position attribute */
	OPENGL_ATTRIBUTE_POSITION,
	/** The normal attribute */
	OPENGL_ATTRIBUTE_NORMAL,
	/** The color attribute */
	OPENGL_ATTRIBUTE_COLOR,
	/** The UV attribute */
	OPENGL_ATTRIBUTE_UV,
	/** The birth attribute */
	OPENGL_ATTRIBUTE_BIRTH,
	/** The angular velocity attribute */
	OPENGL_ATTRIBUTE_ANGULAR_VELOCITY
} OpenGLAttributeLocation;

API void initOpenGLShaders();
API void freeOpenGLShaders();
API bool addOpenGLGlobalShaderUniform(const char *name, OpenGLUniform *uniform);
API bool delOpenGLGlobalShaderUniform(const char *name);
API void refreshOpenGLGlobalShaderUniform(const char *material, const char *globalUniform);
API GLuint createOpenGLShaderFromString(const char *source, GLenum type);
API GLuint createOpenGLShaderFromFile(const char *filename, GLenum type);
API GLuint createOpenGLShaderProgram(GLuint vertexShader, GLuint fragmentShader, bool recycleShaders);

#endif
