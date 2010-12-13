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
#include "api.h"

extern "C" {
#include "mesh.h"
#include "material.h"
}

#include "model.h"

/**
 * Struct to represent an OpenGL model
 */
typedef struct {
	/** The name of the model */
	char *name;
	/** The mesh that belongs to this material */
	OpenGLMesh *mesh;
	/** True if the model should be drawn */
	bool visible;
	/** The material to use before drawing the model */
	char *material;
	/** The base model transformation to which all further modifications are applied */
	Matrix *base_transform;
	/** The current model transformation */
	Matrix *transform;
	/** The translation of the model */
	Vector *translation;
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
 * @param name		the name of the OpenGL model to create
 * @result			true if successful
 */
API bool createOpenGLModel(char *name)
{
	if(g_hash_table_lookup(models, name) != NULL) {
		LOG_ERROR("Failed to create model '%s', a model with that name already exists!", name);
		return false;
	}

	OpenGLModel *model = ALLOCATE_OBJECT(OpenGLModel);
	model->name = strdup(name);
	model->mesh = NULL;
	model->visible = false;
	model->material = NULL;
	model->base_transform = new Matrix(4, 4);
	model->base_transform->identity();
	model->translation = new Vector(3);
	model->translation->clear();

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
 * Attaches a mesh to an OpenGL model and makes it visible
 *
 * @param name		the name of the OpenGL mesh to attach the mesh to
 * @param program	the OpenGLMesh object that should be attached to the model
 * @result			true if successful
 */
API bool attachOpenGLModelMesh(char *name, OpenGLMesh *mesh)
{
	OpenGLModel *model;

	if((model = (OpenGLModel *) g_hash_table_lookup(models, name)) == NULL) {
		LOG_ERROR("Failed to attach a mesh to non existing model '%s'", name);
		return false;
	}

	model->mesh = mesh;
	model->visible = true;

	return true;
}

/**
 * Sets the material to be used before drawing an OpenGL model
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

	OpenGLUniform *uniform;
	if((uniform = (OpenGLUniform *) getOpenGLMaterialUniform(material_name, "model")) == NULL || uniform->type != OPENGL_UNIFORM_MATRIX) {
		LOG_ERROR("Tried to attach material '%s' without a matrix 'model' uniform to model '%s'", material_name, model_name);
		return false;
	}

	model->material = material_name;
	model->transform = uniform->content.matrix_value;

	updateOpenGLModelTransform(model);

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

		if(model->mesh == NULL) {
			LOG_WARNING("Trying to draw visible model '%s' without a mesh attached, skipping", name);
			continue;
		}

		if(model->material != NULL) {
			useOpenGLMaterial(model->material);
		}

		drawOpenGLMesh(model->mesh);
	}
}

/**
 * Updated the transformation matrix for an OpenGL model
 *
 * @param model		the model for which to update the transformation matrix
 */
static void updateOpenGLModelTransform(OpenGLModel *model)
{
	// Don't try to update the transformation if we have no material attached
	if(model->transform == NULL) {
		return;
	}

	*model->transform = *model->base_transform;

	Matrix translationMatrix = Matrix(4, 4).identity();
	translationMatrix(0, 3) = (*model->translation)[0];
	translationMatrix(1, 3) = (*model->translation)[1];
	translationMatrix(1, 3) = (*model->translation)[2];

	*model->transform *= translationMatrix;
}

/**
 * A GDestroyNotify function to free an OpenGL model
 *
 * @param model_p		a pointer to the model to be freed
 */
static void freeOpenGLModel(void *model_p)
{
	OpenGLModel *model = (OpenGLModel *) model_p;

	delete model->base_transform;
	delete model->translation;
	free(model);
}

