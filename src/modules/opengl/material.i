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

#ifndef OPENGL_MATERIAL_H
#define OPENGL_MATERIAL_H

#include <GL/glew.h>
#include "modules/linalg/Matrix.h"
#include "shader.h"
#include "uniform.h"


/**
 * Initializes the OpenGL material store
 */
API void initOpenGLMaterials();

/**
 * Frees the OpenGL material store
 */
API void freeOpenGLMaterials();

/**
 * Creates a new OpenGL material
 *
 * @param name		the name of the OpenGL material to create
 * @result			true if successful
 */
API bool createOpenGLMaterial(const char *name);

/**
 * Creates a new OpenGL material from two shader files
 *
 * @param name					the name of the OpenGL material to create
 * @param vertexShaderFile		the file name of the vertex shader to use for the OpenGL material
 * @param fragmentShaderFile	the file name of the fragment shader to use for the OpenGL material
 * @result						true if successful
 */
API bool createOpenGLMaterialFromFiles(const char *name, const char *vertexShaderFile, const char *fragmentShaderFile);

/**
 * Deletes an OpenGL material
 *
 * @param name		the name of the OpenGL material to delete
 * @result			true if successful
 */
API bool deleteOpenGLMaterial(const char *name);

/**
 * Attaches a shader program to an OpenGL material
 *
 * @param name		the name of the OpenGL material to add the shader program to
 * @param program	the identifier of the OpenGL shader program that should be attached to the material
 * @result			true if successful
 */
API bool attachOpenGLMaterialShaderProgram(const char *name, GLuint program);

/**
 * Returns the OpenGL uniform attachment point for an OpenGL material
 *
 * @param name			the name of the OpenGL material to retrieve the OpenGL uniform attachment point for
 * @result				the OpenGL uniform attachment point for the specified material or NULL on failure
 */
API OpenGLUniformAttachment *getOpenGLMaterialUniforms(const char *name);

/**
 * Uses an OpenGL material for rendering a model
 *
 * @param					the name of the material to use
 * @param modelUniforms		the OpenGL uniform attachment point for model specific uniforms
 * @param model				the model matrix to use for the material
 * @param modelNormal		the normal model matrix to use for the material
 * @result					true if successful
 */
API bool useOpenGLMaterial(const char *name, OpenGLUniformAttachment *uniforms, Matrix *model, Matrix *modelNormal);

/**
 * Check whether an OpenGL material already has a shader attached
 *
 * @param name			the name of the OpenGL material to check
 * @result				true if the material exists and has a shader attached
 */
API bool checkOpenGLMaterialShader(const char *name);

/**
 * Retrieves a list of OpenGL material names
 *
 * @result		a list of OpenGL material names registered, must not be modified but be freed with g_list_free
 */
API GList *getOpenGLMaterials();

#endif
