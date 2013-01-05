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


/**
 * Initializes the OpenGL uniforms
 */
API void initOpenGLUniforms();

/**
 * Frees the OpenGL uniforms
 */
API void freeOpenGLUniforms();

/**
 * Retrieves the global OpenGL uniform attachment point
 *
 * @result			the global OpenGL uniform attachment point
 */
API OpenGLUniformAttachment *getOpenGLGlobalUniforms();

/**
 * Creates an int valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformInt(int value);

/**
 * Creates an int pointer valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformIntPointer(int *value);

/**
 * Creates a float valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformFloat(double value);

/**
 * Creates a float pointer valued OpenGL uniform
 *
 * @param value			the value of the uniform
 * @result				the created uniform
 */
API OpenGLUniform *createOpenGLUniformFloatPointer(float *value);

/**
 * Creates a vector valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be a 4-vector
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformVector(Vector *value);

/**
 * Creates a matrix valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be a 4x4-matrix
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformMatrix(Matrix *value);

/**
 * Creates a texture valued OpenGL uniform
 *
 * @param value			the value of the uniform, must be an OpenGL texture
 * @result				the created uniform or NULL on failure
 */
API OpenGLUniform *createOpenGLUniformTexture(OpenGLTexture *texture);

/**
 * Copies an OpenGL uniform
 *
 * @param uniform			the OpenGL uniform to be copied
 * @result					the duplicated uniform
 */
API OpenGLUniform *copyOpenGLUniform(OpenGLUniform *uniform);

/**
 * Uses a uniform in the current shader program
 *
 * @param uniform		the uniform to use
 * @result				true if successful
 */
API bool useOpenGLUniform(OpenGLUniform *uniform);

/**
 * Creates an OpenGL uniform attachment point
 *
 * @result		the created uniform attachment point
 */
API OpenGLUniformAttachment *createOpenGLUniformAttachment();

/**
 * Attaches an OpenGL uniform to a uniform attachment point
 *
 * @param attachment		the OpenGL uniform attachment point to attach the uniform to
 * @param name				the name of the uniform to be added
 * @param uniform			the uniform to be added
 * @result					true if successful
 */
API bool attachOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name, OpenGLUniform *uniform);

/**
 * Retrieves an OpenGL uniform from an OpenGL uniform attachment point
 *
 * @param attachment		the OpenGL uniform attachment point from which to retrieve an OpenGL uniform
 * @param name				the name of the OpenGL uniform to retrieve
 * @result					the retrieved OpenGL uniform or NULL if no such uniform could be found attached to the attachment point
 */
API OpenGLUniform *getOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name);

/**
 * Detaches an OpeNGL uniform from a uniform attachment point
 *
 * @param attachment			the OpenGL uniform attachment point to detach the uniform from
 * @param name					the name of the uniform to be detached
 * @result						true if successful
 */
API bool detachOpenGLUniform(OpenGLUniformAttachment *attachment, const char *name);

/**
 * Uses all uniforms in an OpenGL uniform attachment point
 *
 * @param attachment			the OpenGL uniform attachment point for which to use all uniforms
 * @param program				the OpenGL shader program for which the uniforms should be used
 * @param textureIndex			a pointer the texture index to start from, will be updated to reflect the new texture index to start from for further attachments
 * @result						true if successful
 */
API bool useOpenGLUniformAttachment(OpenGLUniformAttachment *attachment, GLuint program, unsigned int *textureIndex);

/**
 * Frees an OpenGL uniform attachment point
 *
 * @param attachment			the attachment point to free
 */
API void freeOpenGLUniformAttachment(OpenGLUniformAttachment *attachment);

#endif
