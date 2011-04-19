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
#include "api.h"

extern "C" {
#include "primitive.h"
#include "material.h"
}

#include "model.h"

/**
 * Struct to represent an OpenGL model
 */
typedef struct {
	/** The name of the model */
	char *name;
	/** The primitive that belongs to this model */
	OpenGLPrimitive *primitive;
	/** True if the model should be drawn */
	bool visible;
	/** The material to use before drawing the model */
	char *material;
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
} OpenGLModel;

/**
 * A hash table keeping track of all models
 */
static GHashTable *models;

static void updateOpenGLModelTransform(OpenGLModel *model);
static void freeOpenGLModel(void *model_p);

/**
 * Initializes the OpenGL models
 */
API void initOpenGLModels()
{
	models = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeOpenGLModel);
}

/**
 * Frees the OpenGL models
 */
API void freeOpenGLModels()
{
	g_hash_table_destroy(models);
}


/**
 * Creates a new OpenGL model
 *
 * @param name			the name of the OpenGL model to create
 * @param primitive		the primitive for which to create the model
 * @result				true if successful
 */
API bool createOpenGLModel(char *name, OpenGLPrimitive *primitive)
{
	if(g_hash_table_lookup(models, name) != NULL) {
		LOG_ERROR("Failed to create model '%s', a model with that name already exists!", name);
		return false;
	}

	OpenGLModel *model = ALLOCATE_OBJECT(OpenGLModel);
	model->name = strdup(name);
	model->primitive = primitive;
	model->visible = false;
	model->material = NULL;
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

	g_hash_table_insert(models, model->name, model);

	return true;
}

/**
 * Deletes an OpenGL model
 *
 * @param name		the name of the OpenGL model to delete
 * @result			true if successful
 */
API bool deleteOpenGLModel(char *name)
{
	return g_hash_table_remove(models, name);
}

/**
 * Sets the material to be used before drawing an OpenGL model and makes the model visible
 *
 * @param model_name		the name of the OpenGL model to set the material for
 * @param material_name		the name of the material that should be set for the OpenGL model, or NULL if no material should be used
 * @result					true if successful
 */
API bool attachOpenGLModelMaterial(char *model_name, char *material_name)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to attach material to non existing model '%s'", model_name);
		return false;
	}

	model->material = strdup(material_name);

	OpenGLUniform *modelTransformUniform = createOpenGLUniformMatrix(model->transform);
	OpenGLUniform *modelNormalTransformUniform = createOpenGLUniformMatrix(model->normal_transform);
	detachOpenGLMaterialUniform(material_name, "model"); // remove old uniform if exists
	attachOpenGLMaterialUniform(material_name, "model", modelTransformUniform);
	detachOpenGLMaterialUniform(material_name, "modelNormal"); // remove old uniform if exists
	attachOpenGLMaterialUniform(material_name, "modelNormal", modelNormalTransformUniform);

	model->visible = true;

	return true;
}

/**
 * Sets the translation for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the translation for
 * @param translation		the translation to set for the OpenGL model
 * @result					true if successful
 */
API bool setOpenGLModelTranslation(char *model_name, Vector *translation)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set translation for non existing model '%s'", model_name);
		return false;
	}

	*model->translation = *translation;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the x axis rotation for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param rotation			the rotation in radians to set the model to
 * @result					true if successful
 */
API bool setOpenGLModelRotationX(char *model_name, double rotation)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set rotation for non existing model '%s'", model_name);
		return false;
	}

	model->rotationX = rotation;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the y axis rotation for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param rotation			the rotation in radians to set the model to
 * @result					true if successful
 */
API bool setOpenGLModelRotationY(char *model_name, double rotation)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set rotation for non existing model '%s'", model_name);
		return false;
	}

	model->rotationY = rotation;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the z axis rotation for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param rotation			the rotation in radians to set the model to
 * @result					true if successful
 */
API bool setOpenGLModelRotationZ(char *model_name, double rotation)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set rotation for non existing model '%s'", model_name);
		return false;
	}

	model->rotationZ = rotation;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the x scale for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param scale				the x scale to apply to the model
 * @result					true if successful
 */
API bool setOpenGLModelScaleX(char *model_name, double scale)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set scale for non existing model '%s'", model_name);
		return false;
	}

	model->scaleX = scale;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the y scale for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param scale				the y scale to apply to the model
 * @result					true if successful
 */
API bool setOpenGLModelScaleY(char *model_name, double scale)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set scale for non existing model '%s'", model_name);
		return false;
	}

	model->scaleY = scale;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Sets the z scale for an OpenGL model
 *
 * @param model_name		the name of the OpenGL model to set the rotation for
 * @param scale				the z scale to apply to the model
 * @result					true if successful
 */
API bool setOpenGLModelScaleZ(char *model_name, double scale)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, model_name)) == NULL) {
		LOG_ERROR("Failed to set scale for non existing model '%s'", model_name);
		return false;
	}

	model->scaleZ = scale;
	updateOpenGLModelTransform(model);

	return true;
}

/**
 * Draws all visible OpenGL models to the currently active context
 */
API void drawOpenGLModels()
{
	GHashTableIter iter;
	char *name;
	OpenGLModel *model;
	g_hash_table_iter_init(&iter, models);
	while(g_hash_table_iter_next(&iter, (void **) &name, (void **) &model)) {
		if(!model->visible) {
			continue;
		}

		if(model->primitive == NULL) {
			LOG_WARNING("Trying to draw visible model '%s' without a primitive attached, skipping", name);
			continue;
		}

		if(model->material == NULL) {
			LOG_WARNING("Trying to draw visible model '%s' without a material attached, skipping", name);
			continue;
		}

		if(!useOpenGLMaterial(model->material, model->transform, model->normal_transform)) {
			LOG_WARNING("Failed to use material for visible model '%s'", name);
		}

		if(!drawOpenGLPrimitive(model->primitive)) {
			LOG_WARNING("Drawing of visible model '%s' failed", name);
		}
	}
}

/**
 * Updates all OpenGL models
 *
 * @param dt			the time passed in seconds
 */
API void updateOpenGLModels(double dt)
{
	GHashTableIter iter;
	char *name;
	OpenGLModel *model;
	g_hash_table_iter_init(&iter, models);
	while(g_hash_table_iter_next(&iter, (void **) &name, (void **) &model)) {
		if(model->primitive == NULL) {
			LOG_WARNING("Trying to update model '%s' without a primitive attached, skipping", name);
			continue;
		}

		if(model->primitive->update_function != NULL) { // There is an update function registered
			if(!updateOpenGLPrimitive(model->primitive, dt)) {
				LOG_WARNING("Updating of model '%s' failed", name);
			}
		}
	}
}

/**
 * Updated the transformation matrix for an OpenGL model
 *
 * @param model		the model for which to update the transformation matrix
 */
static void updateOpenGLModelTransform(OpenGLModel *model)
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
 * A GDestroyNotify function to free an OpenGL model
 *
 * @param model_p		a pointer to the model to be freed
 */
static void freeOpenGLModel(void *model_p)
{
	OpenGLModel *model = (OpenGLModel *) model_p;

	free(model->name);
	free(model->material);
	delete model->transform;
	delete model->normal_transform;
	delete model->base_transform;
	delete model->base_normal_transform;
	delete model->translation;
	free(model);
}

