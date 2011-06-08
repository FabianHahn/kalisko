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
#include "uniform.h"
#include "material.h"

/**
 * Struct to represent an OpenGL material
 */
typedef struct {
	/** The name of the material */
	char *name;
	/** The shader program that belongs to this material */
	GLuint program;
	/** The OpenGL uniform attachment point for this material */
	OpenGLUniformAttachment *uniforms;
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
	material->uniforms = createOpenGLUniformAttachment();

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

	return true;
}

/**
 * Returns the OpenGL uniform attachment point for an OpenGL material
 *
 * @param name			the name of the OpenGL material to retrieve the OpenGL uniform attachment point for
 * @result				the OpenGL uniform attachment point for the specified material or NULL on failure
 */
API OpenGLUniformAttachment *getOpenGLMaterialUniforms(const char *name)
{
	OpenGLMaterial *material;
	if((material = g_hash_table_lookup(materials, name)) == NULL) {
		LOG_ERROR("Failed to retrieve OpenGL uniform attachment point for existing material '%s'", name);
		return NULL;
	}

	return material->uniforms;
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

	unsigned int textureIndex = 0;
	useOpenGLUniformAttachment(getOpenGLGlobalUniforms(), material->program, &textureIndex);
	useOpenGLUniformAttachment(material->uniforms, material->program, &textureIndex);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Check whether an OpenGL material already has a shader attached
 *
 * @param name			the name of the OpenGL material to check
 * @result				true if the material exists and has a shader attached
 */
API bool checkOpenGLMaterialShader(const char *name)
{
	OpenGLMaterial *material;

	if((material = g_hash_table_lookup(materials, name)) == NULL) {
		LOG_ERROR("Failed to check non existing material '%s'", name);
		return false;
	}

	return material->program != 0;
}

/**
 * Retrieves a list of OpenGL material names
 *
 * @result		a list of OpenGL material names registered, must not be modified but be freed with g_list_free
 */
API GList *getOpenGLMaterials()
{
	return g_hash_table_get_keys(materials);
}

/**
 * A GDestroyNotify function to free an OpenGL material
 *
 * @param material_p		a pointer to the material to be freed
 */
static void freeOpenGLMaterial(void *material_p)
{
	OpenGLMaterial *material = material_p;

	free(material->name);
	glDeleteProgram(material->program);
	freeOpenGLUniformAttachment(material->uniforms);
	free(material);
}
