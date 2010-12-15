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

#include <GL/glew.h>
#include <glib.h>
#include "dll.h"
#include "modules/opengl/mesh.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "mesh_store.h"

MODULE_NAME("mesh_store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A module providing handlers for writing and reading OpenGL meshes in the store format");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 10, 12), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("linalg", 0, 2, 9));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Creates an OpenGL mesh from a store
 *
 * @param store			the store to parse
 * @result				the parsed OpenGL mesh of NULL on failure
 */
API OpenGLMesh *createOpenGLMeshFromStore(Store *store)
{
	Store *positions;
	if((positions = $(Store *, store, getStorePath)(store, "mesh/vertices/positions")) == NULL || positions->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store: Could not find store list path 'mesh/vertices/positions'");
		$(void, store, freeStore)(store);
		return NULL;
	}

	Store *colors;
	if((colors = $(Store *, store, getStorePath)(store, "mesh/vertices/colors")) == NULL || colors->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store: Could not find store list path 'mesh/vertices/colors'");
		$(void, store, freeStore)(store);
		return NULL;
	}

	Store *triangles;
	if((triangles = $(Store *, store, getStorePath)(store, "mesh/triangles")) == NULL || triangles->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store: Could not find store list path 'mesh/triangles'");
		$(void, store, freeStore)(store);
		return NULL;
	}

	GQueue *pList = positions->content.list;
	GQueue *cList = colors->content.list;
	GQueue *tList = triangles->content.list;

	OpenGLMesh *mesh = $(OpenGLMesh *, opengl, createOpenGLMesh)(g_queue_get_length(pList), g_queue_get_length(tList), GL_STATIC_DRAW);

	// Read vertex positions
	int i = 0;
	for(GList *iter = pList->head; iter != NULL; iter = iter->next, i++) {
		Store *position = iter->data;

		if(position->type != STORE_LIST || g_queue_get_length(position->content.list) != 3) {
			LOG_WARNING("Invalid vertex position for vertex %d in mesh store, replacing by 0/0/0", i);
			mesh->vertices[i].position[0] = 0.0f;
			mesh->vertices[i].position[1] = 0.0f;
			mesh->vertices[i].position[2] = 0.0f;
		} else {
			int j = 0;
			for(GList *piter = position->content.list->head; piter != NULL; piter = piter->next, j++) {
				Store *pval = piter->data;

				if(pval->type != STORE_FLOAT_NUMBER) {
					LOG_WARNING("Invalid vertex position value in component %d of vertex %d in mesh store, replacing by 0", j, i);
					mesh->vertices[i].position[j] = 0;
				} else {
					mesh->vertices[i].position[j] = pval->content.float_number;
				}
			}
		}

		// Reset normal vector
		mesh->vertices[i].normal[0] = 0.0f;
		mesh->vertices[i].normal[1] = 0.0f;
		mesh->vertices[i].normal[2] = 0.0f;
	}

	// Read vertex colors
	i = 0;
	for(GList *iter = cList->head; iter != NULL; iter = iter->next, i++) {
		Store *color = iter->data;
		if(color->type != STORE_LIST || g_queue_get_length(color->content.list) != 4) {
			LOG_WARNING("Invalid vertex color for vertex %d in mesh store, replacing by 0/0/0/0", i);
			mesh->vertices[i].color[0] = 0.0f;
			mesh->vertices[i].color[1] = 0.0f;
			mesh->vertices[i].color[2] = 0.0f;
			mesh->vertices[i].color[3] = 0.0f;
		} else {
			int j = 0;
			for(GList *citer = color->content.list->head; citer != NULL; citer = citer->next, j++) {
				Store *cval = citer->data;

				if(cval->type != STORE_FLOAT_NUMBER) {
					LOG_WARNING("Invalid vertex color value in component %d of vertex %d in mesh store, replacing by 0", j, i);
					mesh->vertices[i].color[j] = 0;
				} else {
					mesh->vertices[i].color[j] = cval->content.float_number;
				}
			}
		}
	}

	// Read triangles
	i = 0;
	for(GList *iter = tList->head; iter != NULL; iter = iter->next, i++) {
		Store *triangle = iter->data;

		if(triangle->type != STORE_LIST) {
			LOG_WARNING("Invalid triangle %d in mesh store, replacing by 0/0/0", i);
			mesh->triangles[i].indices[0] = 0;
			mesh->triangles[i].indices[1] = 0;
			mesh->triangles[i].indices[2] = 0;
		} else {
			int j = 0;
			for(GList *iiter = triangle->content.list->head; iiter != NULL; iiter = iiter->next, j++) {
				Store *ival = iiter->data;

				if(ival->type != STORE_INTEGER || ival->content.integer < 0 || ival->content.integer >= mesh->num_vertices) {
					LOG_WARNING("Invalid index value in component %d of triangle %d in mesh store, replacing by 0", j, i);
					mesh->triangles[i].indices[j] = 0;
				} else {
					mesh->triangles[i].indices[j] = ival->content.integer;
				}
			}
		}
	}

	// Compute normals
	for(i = 0; i < mesh->num_triangles; i++) {
		OpenGLVertex vertex1 = mesh->vertices[mesh->triangles[i].indices[0]];
		OpenGLVertex vertex2 = mesh->vertices[mesh->triangles[i].indices[1]];
		OpenGLVertex vertex3 = mesh->vertices[mesh->triangles[i].indices[2]];

		Vector *v1 = $(Vector *, linalg, createVector3)(vertex1.position[0], vertex1.position[1], vertex1.position[2]);
		Vector *v2 = $(Vector *, linalg, createVector3)(vertex2.position[0], vertex2.position[1], vertex2.position[2]);
		Vector *v3 = $(Vector *, linalg, createVector3)(vertex3.position[0], vertex3.position[1], vertex3.position[2]);
		Vector *e1 = $(Vector *, linalg, diffVectors)(v2, v1);
		Vector *e2 = $(Vector *, linalg, diffVectors)(v3, v1);
		Vector *normal = $(Vector *, linalg, crossVectors)(e1, e2);
		$(void, linalg, normalizeVector)(normal);
		float *normalData = $(float *, linalg, getVectorData)(normal);

		for(int j = i*3; j < (i+1)*3; j++) {
			mesh->vertices[j].normal[0] += normalData[0];
			mesh->vertices[j].normal[1] += normalData[1];
			mesh->vertices[j].normal[2] += normalData[2];
		}

		$(void, linalg, freeVector)(v1);
		$(void, linalg, freeVector)(v2);
		$(void, linalg, freeVector)(v3);
		$(void, linalg, freeVector)(e1);
		$(void, linalg, freeVector)(e2);
		$(void, linalg, freeVector)(normal);
	}

	// Normalize normals
	for(i = 0; i < mesh->num_vertices; i++) {
		OpenGLVertex vertex = mesh->vertices[i];
		Vector *v = $(Vector *, linalg, createVector3)(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
		$(void, linalg, normalizeVector)(v);
		float *vData = $(float *, linalg, getVectorData)(v);

		mesh->vertices[i].normal[0] = vData[0];
		mesh->vertices[i].normal[1] = vData[1];
		mesh->vertices[i].normal[2] = vData[2];

		$(void, linalg, freeVector)(v);
	}

	return mesh;
}

/**
 * Creates a store from an OpenGL mesh
 *
 * @param mesh		the OpenGL mesh to convert to a store
 * @result			the converted store or NULL on failure
 */
API Store *convertOpenGLMeshToStore(OpenGLMesh *mesh)
{
	Store *store = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(store, "mesh", $(Store *, store, createStore)());
	$(bool, store, setStorePath)(store, "mesh/vertices", $(Store *, store, createStore)());

	Store *positions = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/vertices/positions", positions);
	Store *colors = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/vertices/colors", colors);
	Store *triangles = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/triangles", triangles);

	// Write vertices
	for(int i = 0; i < mesh->num_vertices; i++) {
		Store *position = $(Store *, store, createStoreListValue)(NULL);
		Store *color = $(Store *, store, createStoreListValue)(NULL);

		for(int j = 0; j < 4; j++) {
			if(j < 3) {
				g_queue_push_tail(position->content.list, $(Store *, store, createStoreFloatNumberValue)(mesh->vertices[i].position[j]));
			}
			g_queue_push_tail(color->content.list, $(Store *, store, createStoreFloatNumberValue)(mesh->vertices[i].color[j]));
		}

		g_queue_push_tail(positions->content.list, position);
		g_queue_push_tail(colors->content.list, color);
	}

	// Write triangles
	for(int i = 0; i < mesh->num_triangles; i++) {
		Store *triangle = $(Store *, store, createStoreListValue)(NULL);

		for(int j = 0; j < 3; j++) {
			g_queue_push_tail(triangle->content.list, $(Store *, store, createStoreIntegerValue)(mesh->triangles[i].indices[j]));
		}

		g_queue_push_tail(triangles->content.list, triangle);
	}

	return store;
}
