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
#include <assert.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/linalg/Matrix.h"
#include "api.h"
#include "opengl.h"
#include "shader.h"
#include "material.h"

/**
 * Struct to represent an OpenGL material
 */
typedef struct {
	/** The name of the material */
	char *name;
	/** The shader program that belongs to this material */
	GLuint program;
	/** A table of uniforms for the material's shader */
	GHashTable *uniforms;
	/** The model matrix */
	Matrix *model;
	/** The model normal matrix */
	Matrix *modelNormal;
	/** A list of textures added to this material. The positions in the list will correspond to the OpenGL texture units used */
	GQueue *textures;
} OpenGLMaterial;

/**
 * A hash table keeping track of all materials in the material store
 */
static GHashTable *materials;

static void freeOpenGLMaterial(void *material_p);

/**
 * Initializes the OpenGL material store
 */
API void initOpenGLMaterials()
{
	materials = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeOpenGLMaterial);
}

/**
 * Frees the OpenGL material store
 */
API void freeOpenGLMaterials()
{
	g_hash_table_destroy(materials);
}

/**
 * Creates a new OpenGL material
 *
 * @param name		the name of the OpenGL material to create
 * @result			true if successful
 */
API bool createOpenGLMaterial(const char *name)
{
	if(g_hash_table_lookup(materials, name) != NULL) {
		LOG_ERROR("Failed to create material '%s', a material with that name already exists!", name);
		return false;
	}

	OpenGLMaterial *material = ALLOCATE_OBJECT(OpenGLMaterial);
	material->name = strdup(name);
	material->program = 0;
	material->uniforms = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &free);
	material->model = $(Matrix *, linalg, createMatrix)(4, 4);
	material->modelNormal = $(Matrix *, linalg, createMatrix)(4, 4);
	material->textures = g_queue_new();

	g_hash_table_insert(materials, material->name, material);

	return true;
}

/**
 * Deletes an OpenGL material
 *
 * @param name		the name of the OpenGL material to delete
 * @result			true if successful
 */
API bool deleteOpenGLMaterial(const char *name)
{
	return g_hash_table_remove(materials, name);
}

/**
 * Attaches a shader program to an OpenGL material
 *
 * @param name		the name of the OpenGL material to add the shader program to
 * @param program	the identifier of the OpenGL shader program that should be attached to the material
 * @result			true if successful
 */
API bool attachOpenGLMaterialShaderProgram(const char *name, GLuint program)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, name)) == NULL) {
		LOG_ERROR("Failed to attach a shader to non existing material '%s'", name);
		return false;
	}

	material->program = program;

	OpenGLUniform *modelUniform = createOpenGLUniformMatrix(material->model);
	OpenGLUniform *modelNormalUniform = createOpenGLUniformMatrix(material->modelNormal);
	detachOpenGLMaterialUniform(name, "model"); // remove old uniform if exists
	attachOpenGLMaterialUniform(name, "model", modelUniform);
	detachOpenGLMaterialUniform(name, "modelNormal"); // remove old uniform if exists
	attachOpenGLMaterialUniform(name, "modelNormal", modelNormalUniform);

	return true;
}

/**
 * Attaches a uniform to an OpenGL material
 *
 * @param material_name		the name of the OpenGL material to add the uniform to
 * @param uniform_name		the name of the uniform to be added
 * @param uniform			the uniform to be added
 * @result					true if successful
 */
API bool attachOpenGLMaterialUniform(const char *material_name, const char *uniform_name, OpenGLUniform *uniform)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, material_name)) == NULL) {
		LOG_ERROR("Failed to attach a uniform to non existing material '%s'", material_name);
		return false;
	}

	if(g_hash_table_lookup(material->uniforms, uniform_name) != NULL) {
		LOG_ERROR("Failed to attach already existing uniform '%s' to material '%s'", uniform_name, material_name);
		return false;
	}

	if(material->program == 0) {
		LOG_ERROR("Material '%s' doesn't have shader associated yet, aborting attaching of uniform %s", material_name, uniform_name);
		return false;
	}

	GLint location = glGetUniformLocation(material->program, uniform_name);

	if(location == -1) {
		LOG_ERROR("Failed to find uniform location for '%s' in shader program for material '%s'", uniform_name, material_name);
		return false;
	}

	uniform->location = location;

	g_hash_table_insert(material->uniforms, strdup(uniform_name), uniform);

	if(uniform->type == OPENGL_UNIFORM_TEXTURE) { // Textures must be added to the texture list as well
		g_queue_push_tail(material->textures, uniform);
	}

	return true;
}

/**
 * Detaches a uniform from an OpenGL material
 *
 * @param material_name		the name of the OpenGL material to remove the uniform from
 * @param uniform_name		the name of the uniform to be removed
 * @result					true if successful
 */
API bool detachOpenGLMaterialUniform(const char *material_name, const char *uniform_name)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, material_name)) == NULL) {
		LOG_ERROR("Failed to detach a uniform from non existing material '%s'", material_name);
		return false;
	}

	OpenGLUniform *uniform;
	if((uniform = g_hash_table_lookup(material->uniforms, uniform_name)) != NULL && uniform->type == OPENGL_UNIFORM_TEXTURE) { // If the uniform is a texture, remove it from the textures list as well
		g_queue_remove(material->textures, uniform);
	}

	return g_hash_table_remove(material->uniforms, uniform_name);
}

/**
 * Retrieves a uniform from an OpenGL material
 *
 * @param material_name		the name of the OpenGL material for which to retrieve the uniform
 * @param uniform_name		the name of the uniform to be retrieved
 * @result					the retrieved uniform, or NULL if the uniform doesn't exist
 */
API OpenGLUniform *getOpenGLMaterialUniform(const char *material_name, const char *uniform_name)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, material_name)) == NULL) {
		LOG_ERROR("Failed to detach a uniform from non existing material '%s'", material_name);
		return false;
	}

	return g_hash_table_lookup(material->uniforms, uniform_name);
}

/**
 * Uses an OpenGL material for rendering
 *
 * @param				the name of the material to use
 * @param model			the model matrix to use for the material
 * @param modelNormal	the normal model matrix to use for the material
 */
API bool useOpenGLMaterial(const char *name, Matrix *model, Matrix *modelNormal)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, name)) == NULL) {
		LOG_ERROR("Failed to use non existing material '%s'", name);
		return false;
	}

	if(material->program == 0) {
		LOG_ERROR("Material '%s' doesn't have shader associated yet, aborting use", material->name);
		return false;
	}

	glUseProgram(material->program);

	// Assign model matrices
	$(void, linalg, assignMatrix)(material->model, model);
	$(void, linalg, assignMatrix)(material->modelNormal, modelNormal);

	// Iterate over all uniforms for this shaders and use them
	GList *uniforms = g_hash_table_get_values(material->uniforms);
	for(GList *iter = uniforms; iter != NULL; iter = iter->next) {
		OpenGLUniform *uniform = iter->data;

		if(uniform->type != OPENGL_UNIFORM_TEXTURE) { // Texture uniforms are handled seperately
			useOpenGLUniform(uniform);
		}
	}
	g_list_free(uniforms);

	// Iterate over all texture uniforms and first bind them, then use them
	int i = 0;
	for(GList *iter = material->textures->head; iter != NULL; iter = iter->next, i++) {
		OpenGLUniform *uniform = iter->data;
		assert(uniform->type == OPENGL_UNIFORM_TEXTURE);

		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, uniform->content.texture_value->texture);
		uniform->content.texture_value->unit = i; // Set texture unit
		useOpenGLUniform(uniform);
	}

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * A GDestroyNotify function to free an OpenGL material
 *
 * @param material_p		a pointer to the material to be freed
 */
static void freeOpenGLMaterial(void *material_p)
{
	OpenGLMaterial *material = material_p;

	$(void, linalg, freeMatrix)(material->model);
	$(void, linalg, freeMatrix)(material->modelNormal);
	free(material->name);
	glDeleteProgram(material->program);
	g_hash_table_destroy(material->uniforms);
	g_queue_free(material->textures);
}
