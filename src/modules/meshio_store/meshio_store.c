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
#include "modules/meshio/meshio.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#include "modules/linalg/Vector.h"
#include "api.h"

MODULE_NAME("meshio_store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A module providing handlers for writing and reading OpenGL meshes in the store format");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 10, 12), MODULE_DEPENDENCY("meshio", 0, 2, 0), MODULE_DEPENDENCY("store", 0, 6, 7), MODULE_DEPENDENCY("linalg", 0, 2, 9));

static OpenGLMesh *readOpenGLMeshStore(const char *filename);

MODULE_INIT
{
	if(!$(bool, meshio, addMeshIOReadHandler("store", &readOpenGLMeshStore))) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, meshio, deleteMeshIOReadHandler)("store");
}

/**
 * Reads an OpenGL mesh from a store file
 *
 * @param filename			the store file to read from
 * @result					the parsed OpenGL mesh of NULL on failure
 */
static OpenGLMesh *readOpenGLMeshStore(const char *filename)
{
	Store *store;
	if((store = $(Store *, store, parseStoreFile)(filename)) == NULL) {
		LOG_ERROR("Failed to parse mesh store file '%s'", filename);
		return NULL;
	}

	Store *vertices;
	if((vertices = $(Store *, store, getStorePath)(store, "mesh/vertices")) == NULL || vertices->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store file '%s': Could not find store list path 'mesh/vertices'", filename);
		$(void, store, freeStore)(store);
		return NULL;
	}

	Store *triangles;
	if((triangles = $(Store *, store, getStorePath)(store, "mesh/triangles")) == NULL || triangles->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store file '%s': Could not find store list path 'mesh/triangles'", filename);
		$(void, store, freeStore)(store);
		return NULL;
	}

	GQueue *vList = vertices->content.list;
	GQueue *tList = triangles->content.list;

	OpenGLMesh *mesh = $(OpenGLMesh *, opengl, createOpenGLMesh)(g_queue_get_length(vList), g_queue_get_length(tList), GL_STATIC_DRAW);

	// Read vertices
	int i = 0;
	for(GList *iter = vList->head; iter != NULL; iter = iter->next, i++) {
		Store *vertex = iter->data;

		// Read vertex position
		Store *position;
		if(vertex->type != STORE_ARRAY || (position = $(Store *, store, getStorePath)(vertex, "position")) == NULL || position->type != STORE_LIST || g_queue_get_length(position->content.list) != 3) {
			LOG_WARNING("Invalid vertex position for vertex %d in mesh store file '%s', replacing by 0/0/0", i, filename);
			mesh->vertices[i].position[0] = 0.0f;
			mesh->vertices[i].position[1] = 0.0f;
			mesh->vertices[i].position[2] = 0.0f;
		} else {
			int j = 0;
			for(GList *piter = position->content.list->head; piter != NULL; piter = piter->next, j++) {
				Store *pval = piter->data;

				if(pval->type != STORE_FLOAT_NUMBER) {
					LOG_WARNING("Invalid vertex position value in component %d of vertex %d in mesh store file '%s', replacing by 0", j, i, filename);
					mesh->vertices[i].position[j] = 0;
				} else {
					mesh->vertices[i].position[j] = pval->content.float_number;
				}
			}
		}

		// Read vertex color
		Store *color;
		if(vertex->type != STORE_ARRAY || (color = $(Store *, store, getStorePath)(vertex, "color")) == NULL || color->type != STORE_LIST || g_queue_get_length(color->content.list) != 4) {
			LOG_WARNING("Invalid vertex color for vertex %d in mesh store file '%s', replacing by 0/0/0/0", i, filename);
			mesh->vertices[i].color[0] = 0.0f;
			mesh->vertices[i].color[1] = 0.0f;
			mesh->vertices[i].color[2] = 0.0f;
			mesh->vertices[i].color[3] = 0.0f;
		} else {
			int j = 0;
			for(GList *citer = color->content.list->head; citer != NULL; citer = citer->next, j++) {
				Store *cval = citer->data;

				if(cval->type != STORE_FLOAT_NUMBER) {
					LOG_WARNING("Invalid vertex color value in component %d of vertex %d in mesh store file '%s', replacing by 0", j, i, filename);
					mesh->vertices[i].color[j] = 0;
				} else {
					mesh->vertices[i].color[j] = cval->content.float_number;
				}
			}
		}

		// Reset normal vector
		mesh->vertices[i].normal[0] = 0.0f;
		mesh->vertices[i].normal[1] = 0.0f;
		mesh->vertices[i].normal[2] = 0.0f;
	}

	// Read triangles
	i = 0;
	for(GList *iter = tList->head; iter != NULL; iter = iter->next, i++) {
		Store *triangle = iter->data;

		if(triangle->type != STORE_LIST) {
			LOG_WARNING("Invalid triangle %d in mesh store file '%s', replacing by 0/0/0", i, filename);
			mesh->triangles[i].indices[0] = 0;
			mesh->triangles[i].indices[1] = 0;
			mesh->triangles[i].indices[2] = 0;
		} else {
			int j = 0;
			for(GList *iiter = triangle->content.list->head; iiter != NULL; iiter = iiter->next, j++) {
				Store *ival = iiter->data;

				if(ival->type != STORE_INTEGER || ival->content.integer < 0 || ival->content.integer >= mesh->num_vertices) {
					LOG_WARNING("Invalid index value in component %d of triangle %d in mesh store file '%s', replacing by 0", j, i, filename);
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
		mesh->vertices[i].normal[0] = vData[1];
		mesh->vertices[i].normal[0] = vData[2];

		$(void, linalg, freeVector)(v);
	}

	$(void, store, freeStore)(store);

	return mesh;
}
