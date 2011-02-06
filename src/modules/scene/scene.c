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
#include "modules/opengl/mesh.h"
#include "modules/opengl/camera.h"
#include "modules/opengl/model.h"
#include "modules/event/event.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/transform.h"
#include "modules/linalg/store.h"
#include "modules/mesh/io.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "api.h"
#include "scene.h"

MODULE_NAME("scene");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The scene module represents a loadable OpenGL scene that can be displayed and interaced with");
MODULE_VERSION(0, 2, 6);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 11, 3), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("linalg", 0, 3, 0), MODULE_DEPENDENCY("mesh", 0, 4, 0), MODULE_DEPENDENCY("store", 0, 6, 10));

static void freeOpenGLMeshByPointer(void *mesh_p);
static void freeSceneParameterByPointer(void *parameter_p);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

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
	scene->meshes = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeOpenGLMeshByPointer);
	scene->materials = g_queue_new();
	scene->models = g_queue_new();

	// read meshes
	Store *meshes = $(Store *, store, getStorePath)(store, "scene/meshes");
	if(meshes != NULL && meshes->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, meshes->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(value->type == STORE_STRING) {
				GString *meshpath = g_string_new(path_prefix);
				g_string_append(meshpath, value->content.string);
				Mesh *mesh = $(Mesh *, mesh, readMeshFromFile)(meshpath->str);
				g_string_free(meshpath, true);

				if(mesh != NULL) {
					OpenGLMesh *openGLMesh;

					if((openGLMesh = $(OpenGLMesh *, opengl, createOpenGLMesh(mesh, GL_STATIC_DRAW))) != NULL) {
						g_hash_table_insert(scene->meshes, strdup(key), openGLMesh);
						LOG_DEBUG("Added mesh '%s' to scene", key);
					} else {
						LOG_WARNING("Failed to create OpenGLMesh from mesh '%s' when creating scene by store, skipping", key);
						continue;
					}
				} else {
					LOG_WARNING("Failed to read mesh from file for model '%s' when creating scene by store, skipping", key);
					continue;
				}
			} else {
				LOG_WARNING("Failed to read mesh path for model '%s' when creating scene by store, skipping", key);
				continue;
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'meshes' when creating scene by store, skipping mesh loading");
	}

	// read parameters
	Store *parameters = $(Store *, store, getStorePath)(store, "scene/parameters");
	if(parameters != NULL && parameters->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, parameters->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
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
							parameter->content.matrix_value = $(Vector *, linalg, convertStoreToMatrix)(value);
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
					LOG_WARNING("Expected integer, float, vector or matrix value as parameter in 'parameters/%s' when creating scene by store, skipping", key);
				break;
			}

			if(parameter != NULL) {
				g_hash_table_insert(scene->parameters, strdup(key), parameter);
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
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(value->type != STORE_ARRAY) {
				LOG_WARNING("Expected array store value in 'materials/%s' when creating scene by store, skipping", key);
				continue;
			}

			if(!$(bool, opengl, createOpenGLMaterial)(key)) {
				LOG_WARNING("Failed to create OpenGL material '%s' when creatig scene by store, skipping", key);
				break;
			}

			bool hasShader = false;

			// vertex shader
			Store *vertexShaderFile = $(Store *, store, getStorePath)(value, "vertex_shader");
			if(vertexShaderFile != NULL && vertexShaderFile->type == STORE_STRING) {
				GLuint vertexShader;

				GString *vertexShaderPath = g_string_new(path_prefix);
				g_string_append(vertexShaderPath, vertexShaderFile->content.string);
				if((vertexShader = $(GLuint, opengl, createOpenGLShaderFromFile)(vertexShaderPath->str, GL_VERTEX_SHADER)) != 0) {
					// fragment shader
					Store *fragmentShaderFile = $(Store *, store, getStorePath)(value, "fragment_shader");
					if(fragmentShaderFile != NULL && fragmentShaderFile->type == STORE_STRING) {
						GLuint fragmentShader;

						GString *fragmentShaderPath = g_string_new(path_prefix);
						g_string_append(fragmentShaderPath, fragmentShaderFile->content.string);
						if((fragmentShader = $(GLuint, opengl, createOpenGLShaderFromFile)(fragmentShaderPath->str, GL_FRAGMENT_SHADER)) != 0) {
							// shader program
							GLuint program;

							if((program = $(GLuint, opengl, createOpenGLShaderProgram)(vertexShader, fragmentShader, false)) != 0) {
								if($(bool, opengl, attachOpenGLMaterialShaderProgram)(key, program)) {
									hasShader = true;
									LOG_DEBUG("Added shader program to material '%s'", key);
								} else {
									LOG_WARNING("Failed to attach shader program to material '%s' when creatig scene by store, skipping", key);
									glDeleteProgram(program);
								}
							} else {
								LOG_WARNING("Failed to create shader program for material '%s' when creatig scene by store, skipping", key);
							}

							glDeleteShader(vertexShader);
							glDeleteShader(fragmentShader);
						} else {
							LOG_WARNING("Failed to read fragment shader from '%s', for material '%s' when creatig scene by store, skipping", fragmentShaderFile->content.string, key);
							glDeleteShader(vertexShader);
						}

						g_string_free(fragmentShaderPath, true);
					} else {
						LOG_WARNING("Failed to read fragment shader path for material '%s' when creatig scene by store, skipping", key);
						glDeleteShader(vertexShader);
					}
				} else {
					LOG_WARNING("Failed to read vertex shader from '%s', for material '%s' when creatig scene by store, skipping", vertexShaderFile->content.string, key);
				}

				g_string_free(vertexShaderPath, true);
			} else {
				LOG_WARNING("Failed to read vertex shader path for material '%s' when creatig scene by store, skipping", key);
			}

			if(hasShader) {
				// add uniforms
				Store *uniforms = $(Store *, store, getStorePath)(value, "uniforms");
				if(uniforms != NULL && uniforms->type == STORE_ARRAY) {
					GHashTableIter uniformIter;
					char *uniformKey;
					Store *uniformValue;
					g_hash_table_iter_init(&uniformIter, uniforms->content.array);
					while(g_hash_table_iter_next(&uniformIter, (void *) &uniformKey, (void *) &uniformValue)) {
						if(uniformValue->type != STORE_STRING) {
							LOG_WARNING("Expected string store value in 'materials/%s/uniforms/%s' when creating scene by store, skipping", key, uniformKey);
							continue;
						}

						char *parameterName = uniformValue->content.string;
						SceneParameter *parameter;
						if((parameter = g_hash_table_lookup(scene->parameters, parameterName)) != NULL) {
							OpenGLUniform *uniform = ALLOCATE_OBJECT(OpenGLUniform);
							uniform->type = parameter->type;
							uniform->content = parameter->content;
							uniform->location = -1;

							if($(bool, opengl, attachOpenGLMaterialUniform)(key, uniformKey, uniform)) {
								LOG_DEBUG("Added parameter '%s' as uniform '%s' to material '%s'", parameterName, uniformKey, key);
							} else {
								LOG_WARNING("Failed to attach paramter '%s' as uniform '%s' to material '%s' when creating scene by store, skipping", parameterName, uniformKey, key);
								free(uniform);
							}
						} else {
							LOG_WARNING("Failed to add parameter '%s' as uniform '%s' to material '%s' when creating scene by store: No such parameter - skipping", parameterName, uniformKey, key);
						}
					}
				} else {
					LOG_WARNING("Expected array store value in 'materials/%s/uniforms' when creating scene by store, skipping uniform loading", key);
				}
			}

			g_queue_push_head(scene->materials, strdup(key));
		}
	} else {
		LOG_WARNING("Expected array store value in 'materials' when creating scene by store, skipping material loading");
	}

	// read models
	Store *models = $(Store *, store, getStorePath)(store, "scene/models");
	if(models != NULL && models->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, models->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(value->type != STORE_ARRAY) {
				LOG_WARNING("Expected array store value in 'meshes/%s' when creating scene by store, skipping", key);
				continue;
			}

			if(!$(bool, opengl, createOpenGLModel)(key)) {
				LOG_WARNING("Failed to create OpenGL model '%s' when creatig scene by store, skipping", key);
				continue;
			}

			// set mesh
			Store *modelmesh = $(Store *, store, getStorePath)(value, "mesh");
			if(modelmesh != NULL && modelmesh->type == STORE_STRING) {
				char *meshname = modelmesh->content.string;
				OpenGLMesh *openGLMesh;

				if((openGLMesh = g_hash_table_lookup(scene->meshes, meshname)) != NULL) {
					if($(bool, opengl, attachOpenGLModelMesh)(key, openGLMesh)) {
						LOG_DEBUG("Attached mesh '%s' to model '%s'", meshname, key);
					} else {
						LOG_WARNING("Failed to attach mesh '%s' to model '%s' when creating scene by store, skipping", meshname, key);
					}
				} else {
					LOG_WARNING("Failed to add mesh '%s' to model '%s' when creating scene by store: No such model - skipping", meshname, key);
				}
			} else {
				LOG_WARNING("Failed to read mesh for model '%s' when creating scene by store, skipping", key);
			}

			// set material
			Store *modelmaterial = $(Store *, store, getStorePath)(value, "material");
			if(modelmaterial != NULL && modelmaterial->type == STORE_STRING) {
				char *materialname = modelmaterial->content.string;

				if($(bool, opengl, attachOpenGLModelMaterial)(key, materialname)) {
					LOG_DEBUG("Attached material '%s' to model '%s'", materialname, key);
				} else {
					LOG_WARNING("Failed to attach material '%s' to model '%s' when creating scene by store, skipping", materialname, key);
				}
			} else {
				LOG_WARNING("Failed to read material for model '%s' when creating scene by store, skipping", key);
			}

			// set translation
			Store *modeltranslation = $(Store *, store, getStorePath)(value, "translation");
			if(modeltranslation != NULL && modeltranslation->type == STORE_LIST) {
				Vector *translation = $(Vector *, linalg, convertStoreToVector)(modeltranslation);
				if($(bool, opengl, setOpenGLModelTranslation)(key, translation)) {
					LOG_DEBUG("Set translation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set translation for model '%s' when creating scene by store, skipping", key);
				}

				$(void, linalg, freeVector)(translation);
			} else {
				LOG_WARNING("Failed to read translation for model '%s' when creating scene by store, skipping", key);
			}

			// set rotationX
			Store *rotationX = $(Store *, store, getStorePath)(value, "rotationX");
			if(rotationX != NULL && rotationX->type == STORE_FLOAT_NUMBER) {
				if($(bool, opengl, setOpenGLModelRotationX)(key, rotationX->content.float_number)) {
					LOG_DEBUG("Set X rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set X rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read X rotation for model '%s' when creating scene by store, skipping", key);
			}

			// set rotationY
			Store *rotationY = $(Store *, store, getStorePath)(value, "rotationY");
			if(rotationY != NULL && rotationY->type == STORE_FLOAT_NUMBER) {
				if($(bool, opengl, setOpenGLModelRotationY)(key, rotationY->content.float_number)) {
					LOG_DEBUG("Set Y rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Y rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Y rotation for model '%s' when creating scene by store, skipping", key);
			}

			// set rotationZ
			Store *rotationZ = $(Store *, store, getStorePath)(value, "rotationZ");
			if(rotationZ != NULL && rotationZ->type == STORE_FLOAT_NUMBER) {
				if($(bool, opengl, setOpenGLModelRotationZ)(key, rotationZ->content.float_number)) {
					LOG_DEBUG("Set Z rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Z rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Z rotation for model '%s' when creating scene by store, skipping", key);
			}

			// add model name to models list
			g_queue_push_head(scene->models, strdup(key));
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
	// free models
	for(GList *iter = scene->models->head; iter != NULL; iter = iter->next) {
		char *model = iter->data;
		$(void, opengl, deleteOpenGLModel)(model);
		free(model);
	}
	g_queue_free(scene->models);

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
	g_hash_table_destroy(scene->meshes);

	free(scene);
}

/**
 * Frees an OpenGL mesh
 *
 * @param mesh_p		a pointer to the OpenGL mesh to be freed
 */
static void freeOpenGLMeshByPointer(void *mesh_p)
{
	OpenGLMesh *mesh = mesh_p;
	$(void, opengl, freeOpenGLMesh)(mesh);
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
