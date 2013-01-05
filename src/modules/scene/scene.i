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

#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H

#include <glib.h>
#include <GL/glew.h>
#include "modules/opengl/shader.h"
#include "modules/opengl/primitive.h"
#include "modules/opengl/model.h"
#include "modules/store/store.h"

/**
 * Struct that represents a scene parameter which can then be used in OpenGL uniforms
 */
typedef struct {
	/** The type of the scene parameter */
	OpenGLUniformType type;
	/** The content of the scene parameter */
	OpenGLUniformContent content;
} SceneParameter;

/**
 * Struct to represent a scene that can be displayed and interaced with
 */
typedef struct {
	/** The parameters associated with this scene */
	GHashTable *parameters;
	/** The primitives associated with this scene */
	GHashTable *primitives;
	/**	The materials associated with this scene */
	GQueue *materials;
	/**	The models associated with this scene */
	GHashTable *models;
} Scene;


/**
 * Creates a scene from a file
 *
 * @param filename		the file name of the store to read the scene description from
 * @param path_prefix	a prefix to prepend to all file paths in scene
 * @result				the created scene or NULL on failure
 */
API Scene *createScene(char *filename, char *path_prefix);

/**
 * Creates a scene from a store represenation
 *
 * @param store			the store to read the scene description from
 * @param path_prefix	a prefix to prepend to all file paths in scene
 * @result				the created scene
 */
API Scene *createSceneByStore(Store *store, char *path_prefix);

/**
 * Adds an OpenGL primitive to a scene
 *
 * @param scene			the scene to add the primitive to
 * @param key			the name of the primitive to add
 * @param primitive		the primitive to add to the scene (note that the scene takes over control of the primitive)
 * @result				true if successful
 */
API bool addScenePrimitive(Scene *scene, const char *key, OpenGLPrimitive *primitive);

/**
 * Adds an OpenGL texture to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param primitive		the texture to add to the scene (note that the scene takes over control of the texture)
 * @result				true if successful
 */
API bool addSceneTexture(Scene *scene, const char *key, OpenGLTexture *texture);

/**
 * Adds an OpenGL texture loaded from a file to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param filename		the file name from which the texture should be loaded and added to the scene
 * @result				true if successful
 */
API bool addSceneTexture2DFromFile(Scene *scene, const char *key, const char *filename);

/**
 * Adds an OpenGL 2D texture array loaded from a set of files to a scene
 *
 * @param scene			the scene to add the texture to
 * @param key			the name of the texture to add
 * @param filenames		an array of file names from which the texture array should be loaded and added to the scene
 * @param size			the number of elements in the filenames array
 * @result				true if successful
 */
API bool addSceneTexture2DArrayFromFiles(Scene *scene, const char *key, char **filenames, unsigned int size);

/**
 * Adds a scene parameter from a store parameter representation to a scene
 *
 * @param scene			the scene to add the parameter to
 * @param key			the name of the parameter to add
 * @param value			the store representation of the parameter to parse
 * @result				true if successful
 */
API bool addSceneParameterFromStore(Scene *scene, const char *key, Store *store);

/**
 * Adds a scene parameter to a scene
 *
 * @param scene			the scene to add the parameter to
 * @param key			the name of the parameter to add
 * @param parameter		the parameter to add the the scene (note that the scene takes over control of the parameter)
 * @result				true if successful
 */
API bool addSceneParameter(Scene *scene, const char *key, SceneParameter *parameter);

/**
 * Adds a parameter uniform to a scene material
 *
 * @param scene			the scene to add a parameter uniform to one of its materials
 * @param material		the material to which the parameter uniform should be added
 * @param key			the name of the parameter to add as uniform
 * @param name			the name of the uniform to be added to the material
 * @result				true if successful
 */
API bool addSceneMaterialUniformParameter(Scene *scene, const char *material, const char *key, const char *name);

/**
 * Adds a material created from a store configuration to a scene
 *
 * @param scene					the scene to which the created material should be added
 * @param material				the name of the material to be created
 * @param path_prefix			the path prefix to be prepended to all loaded files
 * @param store					the store configuration of the material to create
 * @result						true if successful
 */
API bool addSceneMaterialFromStore(Scene *scene, const char *material, const char *path_prefix, Store *store);

/**
 * Adds a material created from shader files to a scene
 *
 * @param scene					the scene to which the created material should be added
 * @param material				the name of the material to be created
 * @param vertexShaderFile		the file name of the vertex shader to load for the material
 * @param fragmentShaderFile	the file name of the fragment shader to load for the material
 * @result						true if successful
 */
API bool addSceneMaterialFromFiles(Scene *scene, const char *material, const char *vertexShaderFile, const char *fragmentShaderFile);

/**
 * Adds a material to a scene
 *
 * @param scene			the scene to add the material to
 * @param material		the material to add to the scene
 * @result				true if successful
 */
API bool addSceneMaterial(Scene *scene, const char *material);

/**
 * Adds a model to a scene created from a store configuration
 *
 * @param scene			the scene to add the model to
 * @param name			the name of the model to create and add to the scene
 * @param key			the name of the primitive to be used for the created model
 * @result				true if successful
 */
API bool addSceneModelFromStore(Scene *scene, const char *name, Store *store);

/**
 * Adds a model to a scene created from a primitive in the scene
 *
 * @param scene			the scene to add the model to
 * @param name			the name of the model to create and add to the scene
 * @param key			the name of the primitive to be used for the created model
 * @result				the created OpenGL model or NULL on failure
 */
API OpenGLModel *addSceneModelFromPrimitive(Scene *scene, const char *name, const char *key);

/**
 * Adds a model to a scene
 *
 * @param scene			the scene to add the model to
 * @param model			the model to add to the scene
 * @result				true if successful
 */
API bool addSceneModel(Scene *scene, const char *name, OpenGLModel *model);

/**
 * Updates a scene by updating all its OpenGL models
 *
 * @param scene			the scene to update
 * @param dt			the time in seconds that passed since the last update
 */
API void updateScene(Scene *scene, double dt);

/**
 * Draws a scene by drawing all its OpenGL models
 *
 * @param scene			the scene to draw
 */
API void drawScene(Scene *scene);

/**
 * Frees a scene
 *
 * @param scene			the scene to be freed
 */
API void freeScene(Scene *scene);

#endif
