/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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

#include <cassert>
extern "C" {
#include <glib.h>
#include <GL/glew.h>
}

#include "dll.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/transform.h"
#define API

extern "C" {
#include "primitive.h"
#include "material.h"
#include "uniform.h"
}

#include "model.h"

/**
 * Creates a new OpenGL model
 *
 * @param primitive		the primitive for which to create the model
 * @result				true if successful
 */
API OpenGLModel *createOpenGLModel(OpenGLPrimitive *primitive)
{
	OpenGLModel *model = ALLOCATE_OBJECT(OpenGLModel);
	model->primitive = primitive;
	model->visible = false;
	model->material = NULL;
	model->uniforms = NULL;
	model->base_transform = new Matrix(4, 4);
	model->base_transform->identity();
	model->base_normal_transform = new Matrix(4, 4);
	model->base_normal_transform->identity();
	model->translation = new Vector(3);
	model->translation->clear();
	model->rotationX = 0.0f;
	model->rotationY = 0.0f;
	model->rotationZ = 0.0f;
	model->scaleX = 1.0f;
	model->scaleY = 1.0f;
	model->scaleZ = 1.0f;
	model->transform = new Matrix(4, 4);
	model->normal_transform = new Matrix(4, 4);
	model->polygonMode = GL_FILL;

	return model;
}

/**
 * Sets the material to be used before drawing an OpenGL model and makes the model visible
 *
 * @param model			the OpenGL model to set the material for
 * @param name			the name of the material that should be set for the OpenGL model, or NULL if no material should be used
 * @result				true if successful
 */
API bool attachOpenGLModelMaterial(OpenGLModel *model, const char *name)
{
	if(!checkOpenGLMaterialShader(name)) {
		logError("Failed to attach material '%s' without a shader to OpenGL model", name);
		return false;
	}

	if(model->material != NULL) { // reattachment, free old binding
		free(model->material);
		freeOpenGLUniformAttachment(model->uniforms);
	}

	model->material = strdup(name);
	model->uniforms = createOpenGLUniformAttachment();
	model->visible = true;

	OpenGLUniform *modelTransformUniform = createOpenGLUniformMatrix(model->transform);
	attachOpenGLUniform(model->uniforms, "model", modelTransformUniform);
	OpenGLUniform *modelNormalTransformUniform = createOpenGLUniformMatrix(model->normal_transform);
	attachOpenGLUniform(model->uniforms, "modelNormal", modelNormalTransformUniform);

	if(!setupOpenGLPrimitive(model->primitive, model, name)) {
		logError("Setup for OpenGL model with material '%s' failed", name);
		return false;
	}

	return true;
}

/**
 * Draws an OpenGL model to the currently active context
 *
 * @param model			the OpenGL model to draw
 * @param dt			the time in seconds that passed since the last update
 * @result				true if successful
 */
API bool updateOpenGLModel(OpenGLModel *model, double dt)
{
	if(model->primitive == NULL) {
		logError("Failed to update OpenGL model without a primitive attached");
		return false;
	}

	if(!updateOpenGLPrimitive(model->primitive, dt)) {
		logError("Failed to update primitive for OpenGL model");
		return false;
	}

	return true;
}

/**
 * Draws an OpenGL model to the currently active context
 *
 * @param model			the OpenGL model to draw
 * @param options_p		a pointer to custom options to be considered for this draw call
 * @result				true if successful
 */
API bool drawOpenGLModel(OpenGLModel *model, void *options_p)
{
	if(!model->visible) {
		return true; // no need to do anything if the model is invisible
	}

	if(model->primitive == NULL) {
		logError("Failed to draw visible OpenGL model without a primitive attached");
		return false;
	}

	if(model->material == NULL) {
		logError("Failed to draw visible OpenGL model without a material attached");
		return false;
	}

	if(!useOpenGLMaterial(model->material, model->uniforms, model->transform, model->normal_transform)) {
		logError("Failed to use material for OpenGL model");
		return false;
	}

	if(model->polygonMode != GL_FILL) { // the polygon mode is not set to the default, so change it
		glPolygonMode(GL_FRONT_AND_BACK, model->polygonMode);
	}

	if(!drawOpenGLPrimitive(model->primitive, options_p)) {
		logError("Failed to draw primitive for OpenGL model");
		return false;
	}

	if(model->polygonMode != GL_FILL) { // the polygon mode is not set to the default, so revert it back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	return true;
}

/**
 * Updated the transformation matrix for an OpenGL model
 *
 * @param model		the model for which to update the transformation matrix
 */
API void updateOpenGLModelTransform(OpenGLModel *model)
{
	*model->transform = Matrix(4, 4).identity();
	*model->normal_transform = Matrix(4, 4).identity();

	Matrix translationMatrix = Matrix(4, 4).identity();
	translationMatrix(0, 3) = (*model->translation)[0];
	translationMatrix(1, 3) = (*model->translation)[1];
	translationMatrix(2, 3) = (*model->translation)[2];
	*model->transform *= translationMatrix;

	// Apply x rotation
	if(model->rotationX != 0.0f) {
		Matrix *rotation = $(Matrix *, linalg, createRotationMatrixX)(model->rotationX);
		*model->transform *= *rotation;
		*model->normal_transform *= *rotation;
		$(void, linalg, freeMatrix)(rotation);
	}

	// Apply y rotation
	if(model->rotationY != 0.0f) {
		Matrix *rotation = $(Matrix *, linalg, createRotationMatrixY)(model->rotationY);
		*model->transform *= *rotation;
		*model->normal_transform *= *rotation;
		$(void, linalg, freeMatrix)(rotation);
	}

	// Apply z rotation
	if(model->rotationZ != 0.0f) {
		Matrix *rotation = $(Matrix *, linalg, createRotationMatrixZ)(model->rotationZ);
		*model->transform *= *rotation;
		*model->normal_transform *= *rotation;
		$(void, linalg, freeMatrix)(rotation);
	}

	Matrix scale(4, 4);
	Matrix scaleInverse(4, 4);
	scale.identity();
	scaleInverse.identity();

	// Apply x scale
	if(model->scaleX != 1.0f) {
		scale(0, 0) = model->scaleX;
		scaleInverse(0, 0) = 1.0 / model->scaleX;
	}

	// Apply y scale
	if(model->scaleY != 1.0f) {
		scale(1, 1) = model->scaleY;
		scaleInverse(1, 1) = 1.0 / model->scaleY;
	}

	// Apply z scale
	if(model->scaleZ != 1.0f) {
		scale(2, 2) = model->scaleZ;
		scaleInverse(2, 2) = 1.0 / model->scaleZ;
	}

	*model->transform *= scale;
	*model->normal_transform *= scaleInverse;

	*model->transform *= *model->base_transform;
	*model->normal_transform *= *model->base_normal_transform;
}

/**
 * Frees an OpenGL model
 *
 * @param model		the OpenGL model to be freed
 */
API void freeOpenGLModel(OpenGLModel *model)
{
	free(model->material);
	freeOpenGLUniformAttachment(model->uniforms);
	delete model->transform;
	delete model->normal_transform;
	delete model->base_transform;
	delete model->base_normal_transform;
	delete model->translation;
	free(model);
}

