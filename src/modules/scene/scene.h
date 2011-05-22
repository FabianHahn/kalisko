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
#include "modules/store/store.h"
#include "modules/mesh/mesh.h"

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
	GQueue *models;
} Scene;

API Scene *createScene(char *filename, char *path_prefix);
API Scene *createSceneByStore(Store *store, char *path_prefix);
API bool addScenePrimitive(Scene *scene, const char *key, OpenGLPrimitive *primitive);
API bool addSceneTexture(Scene *scene, const char *key, OpenGLTexture *texture);
API bool addSceneTextureFromFile(Scene *scene, const char *key, const char *filename);
API bool addSceneParameterFromStore(Scene *scene, const char *key, Store *store);
API bool addSceneParameter(Scene *scene, const char *key, SceneParameter *parameter);
API void freeScene(Scene *scene);

#endif
