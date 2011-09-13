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

API void initOpenGLMaterials();
API void freeOpenGLMaterials();
API bool createOpenGLMaterial(const char *name);
API bool createOpenGLMaterialFromFiles(const char *name, const char *vertexShaderFile, const char *fragmentShaderFile);
API bool deleteOpenGLMaterial(const char *name);
API bool attachOpenGLMaterialShaderProgram(const char *name, GLuint program);
API OpenGLUniformAttachment *getOpenGLMaterialUniforms(const char *name);
API bool useOpenGLMaterial(const char *name, OpenGLUniformAttachment *uniforms, Matrix *model, Matrix *modelNormal);
API bool checkOpenGLMaterialShader(const char *name);
API GList *getOpenGLMaterials();

#endif