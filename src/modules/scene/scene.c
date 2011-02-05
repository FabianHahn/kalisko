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
#include "modules/mesh/io.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "api.h"
#include "scene.h"

MODULE_NAME("scene");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The scene module represents a loadable OpenGL scene that can be displayed and interaced with");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 11, 1), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("linalg", 0, 2, 9), MODULE_DEPENDENCY("mesh", 0, 4, 0), MODULE_DEPENDENCY("store", 0, 6, 10));

static void freeOpenGLMeshByPointer(void *mesh_p);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Creates a scene from a store represenation
 *
 * @param store			the store to read the scene description from
 * @result				the created scene
 */
API Scene *createSceneByStore(Store *store)
{
	Scene *scene = ALLOCATE_OBJECT(Scene);
	scene->meshes = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeOpenGLMeshByPointer);
	scene->models = g_queue_new();

	// read meshes
	Store *meshes = $(Store *, store, getStorePath)(store, "meshes");
	if(meshes != NULL && meshes->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, meshes->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			char *meshpath = value->content.string;
			Mesh *mesh = $(Mesh *, mesh, readMeshFromFile)(meshpath);
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
				LOG_WARNING("Failed to read mesh for model '%s' when creating scene by store, skipping", key);
				continue;
			}
		}
	} else {
		LOG_WARNING("Expected array store value in 'meshes' when creating scene by store, skipping model loading");
	}

	// read models
	Store *models = $(Store *, store, getStorePath)(store, "models");
	if(models != NULL && models->type == STORE_ARRAY) {
		GHashTableIter iter;
		char *key;
		Store *value;
		g_hash_table_iter_init(&iter, models->content.array);
		while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
			if(!$(bool, opengl, createOpenGLModel)(key)) {
				LOG_WARNING("Failed to create OpenGL model '%s' when creatig scene by store, skipping", key);
				continue;
			}

			Store *modelmesh = $(Store *, store, getStorePath)(value, "mesh");
			if(modelmesh != NULL && modelmesh->type == STORE_STRING) {
				char *meshname = modelmesh->content.string;
				OpenGLMesh *openGLMesh;

				if((openGLMesh = g_hash_table_lookup(scene->meshes, meshname)) != NULL) {
					if($(bool, opengl, attachOpenGLModelMesh)(key, openGLMesh)) {
						LOG_DEBUG("Added mesh '%s' to model '%s'", meshname, key);
					} else {
						LOG_WARNING("Failed to attach mesh '%s' to model '%s' when creating scene by store, skipping", meshname, key);
						continue;
					}
				} else {
					LOG_WARNING("Failed to add mesh '%s' to model '%s' when creating scene by store: No such model - skipping", meshname, key);
					continue;
				}
			} else {
				LOG_WARNING("Failed to read mesh for model '%s' when creating scene by store, skipping", key);
				continue;
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
