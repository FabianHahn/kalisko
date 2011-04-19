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

MODULE_NAME("scene");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The scene module represents a loadable OpenGL scene that can be displayed and interaced with");
MODULE_VERSION(0, 4, 5);
MODULE_BCVERSION(0, 4, 4);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 18, 0), MODULE_DEPENDENCY("linalg", 0, 3, 0), MODULE_DEPENDENCY("image", 0, 4, 0), MODULE_DEPENDENCY("store", 0, 6, 10));

static void freeOpenGLPrimitiveByPointer(void *mesh_p);
static void freeSceneParameterByPointer(void *parameter_p);

MODULE_INIT
{
	initOpenGLPrimitiveSceneParsers();

	return true;
}

MODULE_FINALIZE
{
	freeOpenGLPrimitiveSceneParsers();
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
	scene->models = g_queue_new();

	// read primitives
	Store *primitives = $(Store *, store, getStorePath)(store, "scene/primitives");
	if(primitives != NULL && primitives->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, primitives->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(value->type == STORE_ARRAY) {
				OpenGLPrimitive *primitive;

				if((primitive = parseOpenGLScenePrimitive(path_prefix, value)) != NULL) {
					g_hash_table_insert(scene->primitives, strdup(key), primitive);
					LOG_DEBUG("Added primitive '%s' to scene", key);
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

	// read textures
	Store *textures = $(Store *, store, getStorePath)(store, "scene/textures");
	if(textures != NULL && textures->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, textures->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(value->type == STORE_STRING) {
				GString *texturepath = g_string_new(path_prefix);
				g_string_append(texturepath, value->content.string);
				Image *image = $(Image *, image, readImageFromFile)(texturepath->str);
				g_string_free(texturepath, true);

				if(image != NULL) {
					OpenGLTexture *texture;

					if((texture = $(OpenGLTexture *, opengl, createOpenGLTexture)(image)) != NULL) {
						SceneParameter *parameter = ALLOCATE_OBJECT(SceneParameter);
						parameter->type = OPENGL_UNIFORM_TEXTURE;
						parameter->content.texture_value = texture;

						g_hash_table_insert(scene->parameters, strdup(key), parameter);
						LOG_DEBUG("Added texture '%s' to scene", key);
					} else {
						LOG_WARNING("Failed to create OpenGLTexture from texture image for '%s' when creating scene by store, skipping", key);
						continue;
					}
				} else {
					LOG_WARNING("Failed to read texture file '%s' for '%s' when creating scene by store, skipping", value->content.string, key);
					continue;
				}
			} else {
				LOG_WARNING("Failed to read texture path for '%s' when creating scene by store, skipping", key);
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
					LOG_WARNING("Expected integer, float, vector or matrix value as parameter in 'parameters/%s' when creating scene by store, skipping", key);
				break;
			}

			if(parameter != NULL) {
				g_hash_table_remove(scene->parameters, key); // make sure no texture with that name is inserted
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

			// set primitive
			Store *modelprimitive = $(Store *, store, getStorePath)(value, "primitive");
			if(modelprimitive != NULL && modelprimitive->type == STORE_STRING) {
				char *primitivename = modelprimitive->content.string;
				OpenGLPrimitive *primitive;

				if((primitive = g_hash_table_lookup(scene->primitives, primitivename)) != NULL) {
					if($(bool, opengl, createOpenGLModel)(key, primitive)) {
						LOG_DEBUG("Created OpenGL model '%s' with primitive '%s'", key, primitivename);
					} else {
						LOG_WARNING("Failed to create OpenGL model '%s' with primitive '%s' when creatig scene by store, skipping", key, primitivename);
						continue;
					}
				} else {
					LOG_WARNING("Failed to add primitive '%s' to model '%s' when creating scene by store: No such model - skipping", primitivename, key);
					continue;
				}
			} else {
				LOG_WARNING("Failed to read primitive for model '%s' when creating scene by store, skipping", key);
				continue;
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
			if(rotationX != NULL) {
				if((rotationX->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelRotationX)(key, rotationX->content.float_number)) || (rotationX->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelRotationX)(key, rotationX->content.integer))) {
					LOG_DEBUG("Set X rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set X rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read X rotation for model '%s' when creating scene by store, skipping", key);
			}

			// set rotationY
			Store *rotationY = $(Store *, store, getStorePath)(value, "rotationY");
			if(rotationY != NULL) {
				if((rotationY->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelRotationY)(key, rotationY->content.float_number)) || (rotationY->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelRotationY)(key, rotationY->content.integer))) {
					LOG_DEBUG("Set Y rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Y rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Y rotation for model '%s' when creating scene by store, skipping", key);
			}

			// set rotationZ
			Store *rotationZ = $(Store *, store, getStorePath)(value, "rotationZ");
			if(rotationZ != NULL) {
				if((rotationZ->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelRotationZ)(key, rotationZ->content.float_number)) || (rotationZ->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelRotationZ)(key, rotationZ->content.integer))) {
					LOG_DEBUG("Set Z rotation for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Z rotation for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Z rotation for model '%s' when creating scene by store, skipping", key);
			}

			// set scaleX
			Store *scaleX = $(Store *, store, getStorePath)(value, "scaleX");
			if(scaleX != NULL) {
				if((scaleX->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelScaleX)(key, scaleX->content.float_number)) || (scaleX->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelScaleX)(key, scaleX->content.integer))) {
					LOG_DEBUG("Set X scale for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set X scale for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read X scale for model '%s' when creating scene by store, skipping", key);
			}

			// set scaleY
			Store *scaleY = $(Store *, store, getStorePath)(value, "scaleY");
			if(scaleY != NULL) {
				if((scaleY->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelScaleY)(key, scaleY->content.float_number)) || (scaleY->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelScaleY)(key, scaleY->content.integer))) {
					LOG_DEBUG("Set Y scale for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Y scale for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Y scale for model '%s' when creating scene by store, skipping", key);
			}

			// set scaleZ
			Store *scaleZ = $(Store *, store, getStorePath)(value, "scaleZ");
			if(scaleZ != NULL) {
				if((scaleZ->type == STORE_FLOAT_NUMBER && $(bool, opengl, setOpenGLModelScaleZ)(key, scaleZ->content.float_number)) || (scaleZ->type == STORE_INTEGER && $(bool, opengl, setOpenGLModelScaleZ)(key, scaleZ->content.integer))) {
					LOG_DEBUG("Set Z scale for model '%s'", key);
				} else {
					LOG_WARNING("Failed to set Z scale for model '%s' when creating scene by store, skipping", key);
				}
			} else {
				LOG_WARNING("Failed to read Z scale for model '%s' when creating scene by store, skipping", key);
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
	g_hash_table_destroy(scene->primitives);

	free(scene);
}

/**
 * Frees an OpenGL primitive
 *
 * @param primitive_p		a pointer to the OpenGL primitive to be freed
 */
static void freeOpenGLPrimitiveByPointer(void *primitive_p)
{
	OpenGLPrimitive *primitive = primitive_p;
	$(void, opengl, freeOpenGLPrimitive)(primitive);
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
