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

#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "primitive.h"
#include "uniform.h"

/**
 * Struct to represent an OpenGL model
 */
struct OpenGLModelStruct {
	/** The primitive that belongs to this model */
	OpenGLPrimitive *primitive;
	/** True if the model should be drawn */
	bool visible;
	/** The material to use before drawing the model */
	char *material;
	/** The OpenGL uniform attachment point for model specific uniforms */
	OpenGLUniformAttachment *uniforms;
	/** The base model transformation to which all further modifications are applied */
	Matrix *base_transform;
	/** The inverse base model transformation to which all further modifications are applied */
	Matrix *base_normal_transform;
	/** The current model transformation */
	Matrix *transform;
	/** The current normal model transformation */
	Matrix *normal_transform;
	/** The translation of the model */
	Vector *translation;
	/** The x rotation to apply to the model */
	float rotationX;
	/** The y rotation to apply to the model */
	float rotationY;
	/** The z rotation to apply to the model */
	float rotationZ;
	/** The x scale to apply to the model */
	float scaleX;
	/** The y scale to apply to the model */
	float scaleY;
	/** The z scale to apply to the model */
	float scaleZ;
};

typedef struct OpenGLModelStruct OpenGLModel;

API OpenGLModel *createOpenGLModel(OpenGLPrimitive *primitive);
API bool attachOpenGLModelMaterial(OpenGLModel *model, const char *name);
API void updateOpenGLModelTransform(OpenGLModel *model);
API bool updateOpenGLModel(OpenGLModel *model, double dt);
API bool drawOpenGLModel(OpenGLModel *model);
API void freeOpenGLModel(OpenGLModel *model);

#ifdef __cplusplus
}
#endif

#endif
