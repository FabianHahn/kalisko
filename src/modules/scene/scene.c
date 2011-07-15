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


#include <GL/glew.h>
#include <glib.h>

#include "dll.h"
#include "modules/opengl/opengl.h"
#include "modules/opengl/material.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/camera.h"
#include "modules/opengl/model.h"
#include "modules/opengl/texture.h"
#include "modules/opengl/primitive.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/transform.h"
#include "modules/linalg/store.h"
#include "modules/image/io.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "api.h"
#include "scene.h"
#include "primitive.h"
#include "texture.h"

MODULE_NAME("scene");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The scene module represents a loadable OpenGL scene that can be displayed and interaced with");
MODULE_VERSION(0, 8, 1);
MODULE_BCVERSION(0, 8, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 29, 0), MODULE_DEPENDENCY("linalg", 0, 3, 0), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("store", 0, 6, 10));

static void freeOpenGLPrimitiveByPointer(void *primitive_p);
static void freeSceneParameterByPointer(void *parameter_p);
static void freeOpenGLModelByPointer(void *model_p);

MODULE_INIT
{
	initOpenGLPrimitiveSceneParsers();
	initOpenGLTextureSceneParsers();

	return true;
}

MODULE_FINALIZE
{
	freeOpenGLPrimitiveSceneParsers();
	freeOpenGLTextureSceneParsers();
}

/**
 * Creates a scene from a file
 *
 * @param filename		the file name of the store to read the scene description from
 * @param path_prefix	a prefix to prepend to all file paths in scene
 * @result				the created scene or NULL on failure
 */
API Scene *createScene(char *filename, char *path_prefix)
{
	Store *store;

	if((store = $(Store *, store, parseStoreFile)(filename)) == NULL) {
		LOG_ERROR("Failed to read scene file '%s'", filename);
		return NULL;
	}

	Scene *scene = createSceneByStore(store, path_prefix);
	$(void, store, freeStore)(store);

	return scene;
}

/**
 * Creates a scene from a store represenation
 *
 * @param store			the store to read the scene description from
 * @param path_prefix	a prefix to prepend to all file paths in scene
 * @result				the created scene
 */
API Scene *createSceneByStore(Store *store, char *path_prefix)
{
	Scene *scene = ALLOCATE_OBJECT(Scene);
	scene->parameters = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeSceneParameterByPointer);
	scene->primitives = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeOpenGLPrimitiveByPointer);
	scene->materials = g_queue_new();
	scene->models = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeOpenGLModelByPointer);

	// read textures
	Store *textures = $(Store *, store, getStorePath)(store, "scene/textures");
	if(textures != NULL && textures->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, textures->content.array);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			if(value->type == STORE_ARRAY) {
				OpenGLTexture *texture;

				if((texture = parseOpenGLSceneTexture(scene, path_prefix, key, value)) != NULL) {
					if(addSceneTexture(scene, key, texture)) {
						LOG_DEBUG("Added texture '%s' to scene", key);
					}
				} else {
					LOG_WARNING("Failed to parse texture in 'texture/%s' for scene, skipping", key);
					continue;
				}
			} else {
				LOG_WARNING("Expected array store value in 'texture/%s' when parsing scene texture, skipping", key);
				continue;
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'textures' when creating scene by store, skipping texture loading");
	}

	// read parameters
	Store *parameters = $(Store *, store, getStorePath)(store, "scene/parameters");
	if(parameters != NULL && parameters->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, parameters->content.array);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			if(addSceneParameterFromStore(scene, key, value)) {
				LOG_DEBUG("Added scene parameter '%s'", key);
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'meshes' when creating scene by store, skipping mesh loading");
	}

	// read materials
	Store *materials = $(Store *, store, getStorePath)(store, "scene/materials");
	if(materials != NULL && materials->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, materials->content.array);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			if(value->type != STORE_ARRAY) {
				LOG_WARNING("Expected array store value in 'materials/%s' when creating scene by store, skipping", key);
				continue;
			}

			if(addSceneMaterialFromStore(scene, key, path_prefix, value)) {
				LOG_DEBUG("Added material '%s' to scene", key);
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'materials' when creating scene by store, skipping material loading");
	}

	// read primitives
	Store *primitives = $(Store *, store, getStorePath)(store, "scene/primitives");
	if(primitives != NULL && primitives->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, primitives->content.array);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			if(value->type == STORE_ARRAY) {
				OpenGLPrimitive *primitive;

				if((primitive = parseOpenGLScenePrimitive(scene, path_prefix, key, value)) != NULL) {
					if(addScenePrimitive(scene, key, primitive)) {
						LOG_DEBUG("Added primitive '%s' to scene", key);
					}
				} else {
					LOG_WARNING("Failed to parse primitive in 'primitives/%s' for scene, skipping", key);
					continue;
				}
			} else {
				LOG_WARNING("Expected array store value in 'primitives/%s' when parsing scene primitive, skipping", key);
				continue;
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'primitives' when creating scene by store, skipping primitive loading");
	}

	// read models
	Store *models = $(Store *, store, getStorePath)(store, "scene/models");
	if(models != NULL && models->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, models->content.array);
		while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
			if(value->type != STORE_ARRAY) {
				LOG_WARNING("Expected array store value in 'meshes/%s' when creating scene by store, skipping", key);
				continue;
			}

			if(addSceneModelFromStore(scene, key, value)) {
				LOG_DEBUG("Added model '%s' to scene", key);
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'models' when creating scene by store, skipping model loading");
	}

	return scene;
}

/**
 * Frees a scene
 *
 * @param scene			the scene to be freed
 */
API void freeScene(Scene *scene)
{
	// free materials
	for(GList *iter = scene->materials->head; iter != NULL; iter = iter->next) {
		char *material = iter->data;
		$(void, opengl, deleteOpenGLMaterial)(material);
		free(material);
	}
	g_queue_free(scene->materials);

	// free parameters
	g_hash_table_destroy(scene->parameters);

	// free meshes
	g_hash_table_destroy(scene->primitives);

	// free models
	g_hash_table_destroy(scene->models);

	free(scene);
}

/**
 * Adds an OpenGL primitive to a scene
 *
 * @param scene			the scene to add the primitive to
 * @param key			the name of the primitive to add
 * @param primitive		the primitive to add to the scene (note that the scene takes over control of the primitive)
 * @result				true if successful
 */
API bool addScenePrimitive(Scene *scene, const char *key, OpenGLPrimitive *primitive)
{
	if(g_hash_table_lookup(scene->primitives, key) != NULL) {
		LOG_ERROR("Failed to add primitive '%s' to scene, a primitive with that name already exists!", key);
		return false;
	}

	g_hash_table_insert(scene->primitives, strdup(key), primitive);
	return true;
}

/**
 * Adds an OpenGL texture to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param primitive		the texture to add to the scene (note that the scene takes over control of the texture)
 * @result				true if successful
 */
API bool addSceneTexture(Scene *scene, const char *key, OpenGLTexture *texture)
{
	SceneParameter *parameter = ALLOCATE_OBJECT(SceneParameter);
	parameter->type = OPENGL_UNIFORM_TEXTURE;
	parameter->content.texture_value = texture;

	if(!addSceneParameter(scene, key, parameter)) {
		free(parameter);
		return false;
	}

	return true;
}

/**
 * Adds an OpenGL texture loaded from a file to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param filename		the file name from which the texture should be loaded and added to the scene
 * @result				true if successful
 */
API bool addSceneTexture2DFromFile(Scene *scene, const char *key, const char *filename)
{
	Image *image;
	if((image = $(Image *, image, readImageFromFile)(filename)) != NULL) {
		OpenGLTexture *texture;
		if((texture = $(OpenGLTexture *, opengl, createOpenGLTexture2D)(image, true)) != NULL) {
			if(!addSceneTexture(scene, key, texture)) {
				$(void, opengl, freeOpenGLTexture)(texture);
				return false;
			}
		} else {
			LOG_ERROR("Failed to create OpenGL texture '%s' from '%s' for scene", key, filename);
			$(void, image, freeImage)(image);
			return false;
		}
	} else {
		LOG_ERROR("Failed to read texture '%s' from '%s'", key, filename);
		return false;
	}

	return true;
}

/**
 * Adds an OpenGL 2D texture array loaded from a set of files to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param filenames		an array of file names from which the texture array should be loaded and added to the scene
 * @param size			the number of elements in the filenames array
 * @result				true if successful
 */
API bool addSceneTexture2DArrayFromFiles(Scene *scene, const char *key, char **filenames, unsigned int size)
{
	// read images
	GPtrArray *images = g_ptr_array_new();
	for(unsigned int i = 0; i < size; i++) {
		Image *image;
		if((image = $(Image *, image, readImageFromFile)(filenames[i])) != NULL) {
			g_ptr_array_add(images, image);
		} else {
			LOG_WARNING("Failed to read image %d from '%s' for 2D texture array to be added to scene, skipping", i, filenames[i]);
		}
	}

	// create opengl texture array
	OpenGLTexture *texture = $(OpenGLTexture *, opengl, createOpenGLTexture2DArray)((Image **) images->pdata, images->len, true);

	// free images
	for(unsigned int i = 0; i < size; i++) {
		$(void, image, freeImage)(images->pdata[i]);
	}
	g_ptr_array_free(images, true);

	if(texture == NULL) {
		return false;
	}

	// add it to scene
	if(!addSceneTexture(scene, key, texture)) {
		$(void, opengl, freeOpenGLTexture)(texture);
		return false;
	}

	return true;
}

/**
 * Adds a scene parameter from a store parameter representation to a scene
 *
 * @param scene			the scene to add the parameter to
 * @param key			the name of the parameter to add
 * @param value			the store representation of the parameter to parse
 * @result				true if successful
 */
API bool addSceneParameterFromStore(Scene *scene, const char *key, Store *value)
{
	SceneParameter *parameter = ALLOCATE_OBJECT(SceneParameter);

	switch(value->type) {
		case STORE_INTEGER:
			parameter->type = OPENGL_UNIFORM_INT;
			parameter->content.int_value = value->content.integer;
		break;
		case STORE_FLOAT_NUMBER:
			parameter->type = OPENGL_UNIFORM_FLOAT;
			parameter->content.float_value = value->content.float_number;
		break;
		case STORE_LIST:
			if(g_queue_get_length(value->content.list) > 0) {
				Store *firstelement = value->content.list->head->data;
				if(firstelement->type == STORE_LIST) { // assume matrix
					parameter->type = OPENGL_UNIFORM_MATRIX;
					parameter->content.matrix_value = $(Matrix *, linalg, convertStoreToMatrix)(value);
				} else { // assume vector
					parameter->type = OPENGL_UNIFORM_VECTOR;
					parameter->content.vector_value = $(Vector *, linalg, convertStoreToVector)(value);
				}
			} else {
				parameter->type = OPENGL_UNIFORM_VECTOR;
				parameter->content.vector_value = $(Vector *, linalg, createVector)(0);
			}
		break;
		default:
			free(parameter);
			parameter = NULL;
			LOG_WARNING("Expected integer, float, vector or matrix value for parameter '%s' for scene, skipping", key);
		break;
	}

	if(parameter != NULL) {
		if(!addSceneParameter(scene, key, parameter)) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

/**
 * Adds a scene parameter to a scene
 *
 * @param scene			the scene to add the parameter to
 * @param key			the name of the parameter to add
 * @param parameter		the parameter to add the the scene (note that the scene takes over control of the parameter)
 * @result				true if successful
 */
API bool addSceneParameter(Scene *scene, const char *key, SceneParameter *parameter)
{
	if(g_hash_table_lookup(scene->parameters, key) != NULL) {
		LOG_ERROR("Failed to add parameter '%s' to scene, a parameter with that name already exists!", key);
		return false;
	}

	g_hash_table_insert(scene->parameters, strdup(key), parameter);
	return true;
}

/**
 * Adds a parameter uniform to a scene material
 *
 * @param scene			the scene to add a parameter uniform to one of its materials
 * @param material		the material to which the parameter uniform should be added
 * @param key			the name of the parameter to add as uniform
 * @param name			the name of the uniform to be added to the material
 * @result				true if successful
 */
API bool addSceneMaterialUniformParameter(Scene *scene, const char *material, const char *key, const char *name)
{
	SceneParameter *parameter;
	if((parameter = g_hash_table_lookup(scene->parameters, key)) != NULL) {
		OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
		uniform->type = parameter->type;
		uniform->content = parameter->content;
		uniform->location = -1;

		OpenGLUniformAttachment *uniforms = $(OpenGLUniformAttachment *, opengl, getOpenGLMaterialUniforms)(material);
		if(uniforms == NULL || !$(bool, opengl, attachOpenGLUniform)(uniforms, name, uniform)) {
			LOG_ERROR("Failed to attach parameter '%s' as uniform '%s' to material '%s'", key, name, material);
			free(uniform);
			return false;
		}
	} else {
		LOG_ERROR("Failed to attach parameter '%s' as uniform '%s' to material '%s': No such parameter found", key, name, material);
		return false;
	}

	return true;
}

/**
 * Adds a material created from a store configuration to a scene
 *
 * @param scene					the scene to which the created material should be added
 * @param material				the name of the material to be created
 * @param path_prefix			the path prefix to be prepended to all loaded files
 * @param store					the store configuration of the material to create
 * @result						true if successful
 */
API bool addSceneMaterialFromStore(Scene *scene, const char *material, const char *path_prefix, Store *store)
{
	// vertex shader path
	Store *vertexShaderParam = $(Store *, store, getStorePath)(store, "vertex_shader");
	if(vertexShaderParam == NULL || vertexShaderParam->type != STORE_STRING) {
		LOG_ERROR("Failed to read vertex_shader path property for material '%s' to be added to scene", material);
		return false;
	}

	GString *vertexShaderPath = g_string_new(path_prefix);
	g_string_append(vertexShaderPath, vertexShaderParam->content.string);

	// fragment shader path
	Store *fragmentShaderParam = $(Store *, store, getStorePath)(store, "fragment_shader");
	if(fragmentShaderParam == NULL || fragmentShaderParam->type != STORE_STRING) {
		LOG_ERROR("Failed to read fragment_shader path property for material '%s' to be added to scene", material);
		g_string_free(vertexShaderPath, true);
		return false;
	}

	GString *fragmentShaderPath = g_string_new(path_prefix);
	g_string_append(fragmentShaderPath, fragmentShaderParam->content.string);

	bool ret = addSceneMaterialFromFiles(scene, material, vertexShaderPath->str, fragmentShaderPath->str);
	g_string_free(vertexShaderPath, true);
	g_string_free(fragmentShaderPath, true);

	if(ret) {
		// add uniforms
		Store *uniforms = $(Store *, store, getStorePath)(store, "uniforms");
		if(uniforms != NULL && uniforms->type == STORE_ARRAY) {
			GHashTableIter uniformIter;
			char *uniformKey;
			Store *uniformValue;
			g_hash_table_iter_init(&uniformIter, uniforms->content.array);
			while(g_hash_table_iter_next(&uniformIter, (void **) &uniformKey, (void **) &uniformValue)) {
				if(uniformValue->type != STORE_STRING) {
					LOG_WARNING("Expected string store value in 'uniforms/%s' for material '%s' to be added to scene, skipping", uniformKey, material);
					continue;
				}

				if(addSceneMaterialUniformParameter(scene, material, uniformValue->content.string, uniformKey)) {
					LOG_DEBUG("Added parameter '%s' as uniform '%s' to material '%s' to be added to scene", uniformValue->content.string, uniformKey, material);
				}
			}
		} else {
			LOG_DEBUG("No uniforms specified for material '%s' to be added to scene", material);
		}
	}

	return ret;
}

/**
 * Adds a material created from shader files to a scene
 *
 * @param scene					the scene to which the created material should be added
 * @param material				the name of the material to be created
 * @param vertexShaderFile		the file name of the vertex shader to load for the material
 * @param fragmentShaderFile	the file name of the fragment shader to load for the material
 * @result						true if successful
 */
API bool addSceneMaterialFromFiles(Scene *scene, const char *material, const char *vertexShaderFile, const char *fragmentShaderFile)
{
	// create the material
	if(!$(bool, opengl, createOpenGLMaterial)(material)) {
		LOG_ERROR("Failed to create OpenGL material '%s' to be added to scene", material);
		return false;
	}

	// load vertex shader
	GLuint vertexShader;
	if((vertexShader = $(GLuint, opengl, createOpenGLShaderFromFile)(vertexShaderFile, GL_VERTEX_SHADER)) == 0) {
		LOG_ERROR("Failed to read vertex shader from '%s' for material '%s' to be added to scene", vertexShaderFile, material);
		$(bool, opengl, deleteOpenGLMaterial)(material);
		return false;
	}

	// load fragment shader
	GLuint fragmentShader;
	if((fragmentShader = $(GLuint, opengl, createOpenGLShaderFromFile)(fragmentShaderFile, GL_FRAGMENT_SHADER)) == 0) {
		LOG_ERROR("Failed to read fragment shader from '%s' for material '%s' to be added to scene", fragmentShaderFile, material);
		glDeleteShader(vertexShader);
		$(bool, opengl, deleteOpenGLMaterial)(material);
		return false;
	}

	// link them to a shader program
	GLuint program;
	if((program = $(GLuint, opengl, createOpenGLShaderProgram)(vertexShader, fragmentShader, false)) == 0) {
		LOG_ERROR("Failed to create OpenGL shader program for material '%s' to be added to scene", material);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		$(bool, opengl, deleteOpenGLMaterial)(material);
		return false;
	}

	// attach shader program to material
	if(!$(bool, opengl, attachOpenGLMaterialShaderProgram)(material, program)) {
		LOG_ERROR("Failed to attach shader program to material '%s' to be added to scene", material);
		glDeleteProgram(program);
		$(bool, opengl, deleteOpenGLMaterial)(material);
		return false;
	}

	// add material to scene
	if(!addSceneMaterial(scene, material))
	{
		$(bool, opengl, deleteOpenGLMaterial)(material);
		return false;
	}

	return true;
}

/**
 * Adds a material to a scene
 *
 * @param scene			the scene to add the material to
 * @param material		the material to add to the scene
 * @result				true if successful
 */
API bool addSceneMaterial(Scene *scene, const char *material)
{
	for(GList *iter = scene->materials->head; iter != NULL; iter = iter->next) {
		if(g_strcmp0(material, iter->data) == 0) {
			LOG_ERROR("Failed to add material '%s' to scene: The scene already contains this material", material);
			return false;
		}
	}

	g_queue_push_tail(scene->materials, strdup(material));
	return true;
}

/**
 * Adds a model to a scene created from a store configuration
 *
 * @param scene			the scene to add the model to
 * @param name			the name of the model to create and add to the scene
 * @param key			the name of the primitive to be used for the created model
 * @result				true if successful
 */
API bool addSceneModelFromStore(Scene *scene, const char *name, Store *store)
{
	// set primitive
	Store *modelprimitive = $(Store *, store, getStorePath)(store, "primitive");
	if(modelprimitive == NULL || modelprimitive->type != STORE_STRING) {
		LOG_ERROR("Failed to read 'primitive' string store value for model '%s' to be added to scene", name);
		return false;
	}

	OpenGLModel *model;
	if((model = addSceneModelFromPrimitive(scene, name, modelprimitive->content.string)) == NULL) {
		return false;
	}

	// set material
	Store *modelmaterial = $(Store *, store, getStorePath)(store, "material");
	if(modelmaterial != NULL && modelmaterial->type == STORE_STRING) {
		char *materialname = modelmaterial->content.string;

		if($(bool, opengl, attachOpenGLModelMaterial)(model, materialname)) {
			LOG_DEBUG("Attached material '%s' to model '%s'", materialname, name);
		} else {
			LOG_WARNING("Failed to attach material '%s' to model '%s' to be added to scene, skipping", materialname, name);
		}
	} else {
		LOG_WARNING("Failed to read material for model '%s' to be added to scene, skipping", name);
	}

	// set translation
	Store *modeltranslation = $(Store *, store, getStorePath)(store, "translation");
	if(modeltranslation != NULL && modeltranslation->type == STORE_LIST) {
		Vector *translation = $(Vector *, linalg, convertStoreToVector)(modeltranslation);
		$(void, linalg, assignVector)(model->translation, translation);
		LOG_DEBUG("Set translation for model '%s'", name);
		$(void, linalg, freeVector)(translation);
	}

	// set rotationX
	Store *rotationX = $(Store *, store, getStorePath)(store, "rotationX");
	if(rotationX != NULL) {
		switch(rotationX->type) {
			case STORE_FLOAT_NUMBER:
				model->rotationX = rotationX->content.float_number;
				LOG_DEBUG("Set X rotation for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->rotationX = rotationX->content.integer;
				LOG_DEBUG("Set X rotation for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set X rotation for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	// set rotationY
	Store *rotationY = $(Store *, store, getStorePath)(store, "rotationY");
	if(rotationY != NULL) {
		switch(rotationY->type) {
			case STORE_FLOAT_NUMBER:
				model->rotationX = rotationY->content.float_number;
				LOG_DEBUG("Set Y rotation for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->rotationY = rotationY->content.integer;
				LOG_DEBUG("Set Y rotation for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set Y rotation for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	// set rotationZ
	Store *rotationZ = $(Store *, store, getStorePath)(store, "rotationZ");
	if(rotationZ != NULL) {
		switch(rotationZ->type) {
			case STORE_FLOAT_NUMBER:
				model->rotationZ = rotationZ->content.float_number;
				LOG_DEBUG("Set Z rotation for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->rotationZ = rotationZ->content.integer;
				LOG_DEBUG("Set Z rotation for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set Z rotation for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	// set scaleX
	Store *scaleX = $(Store *, store, getStorePath)(store, "scaleX");
	if(scaleX != NULL) {
		switch(scaleX->type) {
			case STORE_FLOAT_NUMBER:
				model->scaleX = scaleX->content.float_number;
				LOG_DEBUG("Set X scale for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->scaleX = scaleX->content.integer;
				LOG_DEBUG("Set X scale for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set X scale for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	// set scaleY
	Store *scaleY = $(Store *, store, getStorePath)(store, "scaleY");
	if(scaleY != NULL) {
		switch(scaleY->type) {
			case STORE_FLOAT_NUMBER:
				model->scaleY = scaleY->content.float_number;
				LOG_DEBUG("Set Y scale for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->scaleY = scaleY->content.integer;
				LOG_DEBUG("Set Y scale for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set Y scale for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	// set scaleZ
	Store *scaleZ = $(Store *, store, getStorePath)(store, "scaleZ");
	if(scaleZ != NULL) {
		switch(scaleZ->type) {
			case STORE_FLOAT_NUMBER:
				model->scaleZ = scaleZ->content.float_number;
				LOG_DEBUG("Set Z scale for model '%s'", name);
			break;
			case STORE_INTEGER:
				model->scaleZ = scaleZ->content.integer;
				LOG_DEBUG("Set Z scale for model '%s'", name);
			break;
			default:
				LOG_WARNING("Failed to set Z scale for model '%s' to be added to scene, skipping", name);
			break;
		}
	}

	return true;
}

/**
 * Adds a model to a scene created from a primitive in the scene
 *
 * @param scene			the scene to add the model to
 * @param name			the name of the model to create and add to the scene
 * @param key			the name of the primitive to be used for the created model
 * @result				the created OpenGL model or NULL on failure
 */
API OpenGLModel *addSceneModelFromPrimitive(Scene *scene, const char *name, const char *key)
{
	OpenGLPrimitive *primitive;
	if((primitive = g_hash_table_lookup(scene->primitives, key)) == NULL) {
		LOG_ERROR("Failed create model '%s' from primitive '%s' to be added to scene: No such primitive found in scene", name, key);
		return NULL;
	}

	OpenGLModel *model;
	if((model = $(OpenGLModel *, opengl, createOpenGLModel)(primitive)) == NULL) {
		return NULL;
	}

	if(!addSceneModel(scene, name, model)) {
		$(void, opengl, freeOpenGLModel)(model);
		return NULL;
	}

	return model;
}

/**
 * Adds a model to a scene
 *
 * @param scene			the scene to add the model to
 * @param model			the model to add to the scene
 * @result				true if successful
 */
API bool addSceneModel(Scene *scene, const char *name, OpenGLModel *model)
{
	if(g_hash_table_lookup(scene->models, name) != NULL) {
		LOG_ERROR("Failed to add model '%s' to scene: A model with that name already exists", name);
		return false;
	}

	// Insert the model
	g_hash_table_insert(scene->models, strdup(name), model);

	return true;
}

/**
 * Updates a scene by updating all its OpenGL models
 *
 * @param scene			the scene to update
 * @param dt			the time in seconds that passed since the last update
 */
API void updateScene(Scene *scene, double dt)
{
	GHashTableIter iter;
	char *name = NULL;
	OpenGLModel *model = NULL;
	g_hash_table_iter_init(&iter, scene->models);
	while(g_hash_table_iter_next(&iter, (void **) name, (void **) model)) { // loop over all models
		if(!$(bool, opengl, updateOpenGLModel)(model, dt)) {
			LOG_WARNING("Failed to update model '%s' in scene", name);
		}
	}
}

/**
 * Draws a scene by drawing all its OpenGL models
 *
 * @param scene			the scene to draw
 */
API void drawScene(Scene *scene)
{
	GHashTableIter iter;
	char *name = NULL;
	OpenGLModel *model = NULL;
	g_hash_table_iter_init(&iter, scene->models);
	while(g_hash_table_iter_next(&iter, (void **) name, (void **) model)) { // loop over all models
		if(!$(bool, opengl, drawOpenGLModel)(model)) {
			LOG_WARNING("Failed to draw model '%s' in scene", name);
		}
	}
}

/**
 * Frees an OpenGL primitive
 *
 * @param primitive_p		a pointer to the OpenGL primitive to be freed
 */
static void freeOpenGLPrimitiveByPointer(void *primitive_p)
{
	OpenGLPrimitive *primitive = primitive_p;
	freeOpenGLPrimitive(primitive);
}

/**
 * Frees a scene parameter
 *
 * @param parameter_p		a pointer to the scene parameter to be freed
 */
static void freeSceneParameterByPointer(void *parameter_p)
{
	SceneParameter *parameter = parameter_p;
	switch(parameter->type) {
		case OPENGL_UNIFORM_TEXTURE:
			$(void, opengl, freeOpenGLTexture)(parameter->content.texture_value);
		break;
		case OPENGL_UNIFORM_VECTOR:
			$(void, linalg, freeVector)(parameter->content.vector_value);
		break;
		case OPENGL_UNIFORM_MATRIX:
			$(void, linalg, freeMatrix)(parameter->content.matrix_value);
		break;
		default:
			// nothing to free
		break;
	}

	free(parameter);
}

/**
 * Frees an OpenGL model
 *
 * @param model		a pointer to the OpenGL model to be freed
 */
static void freeOpenGLModelByPointer(void *model_p)
{
	OpenGLModel *model = model_p;
	$(void, opengl, freeOpenGLModel)(model);
}
