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
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#define API
#include "mesh.h"
#include "store.h"

/**
 * Creates a mesh from a store
 *
 * @param store			the store to parse
 * @result				the parsed mesh of NULL on failure
 */
API Mesh *createMeshFromStore(Store *store)
{
	Store *positions;
	if((positions = $(Store *, store, getStorePath)(store, "mesh/vertices/positions")) == NULL || positions->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store: Could not find store list path 'mesh/vertices/positions'");
		return NULL;
	}

	Store *triangles;
	if((triangles = $(Store *, store, getStorePath)(store, "mesh/triangles")) == NULL || triangles->type != STORE_LIST) {
		LOG_ERROR("Failed to parse mesh store: Could not find store list path 'mesh/triangles'");
		return NULL;
	}

	Store *colors;
	GQueue *cList = NULL;
	if((colors = $(Store *, store, getStorePath)(store, "mesh/vertices/colors")) == NULL || colors->type != STORE_LIST) {
		LOG_WARNING("Parsed mesh store doesn't seem to have color values stored in 'mesh/vertices/colors', skipping");
	} else {
		 cList = colors->content.list;
	}

	Store *uvs;
	GQueue *uvList = NULL;
	if((uvs = $(Store *, store, getStorePath)(store, "mesh/vertices/uvs")) == NULL || colors->type != STORE_LIST) {
		LOG_WARNING("Parsed mesh store doesn't seem to have UV coordinate values stored in 'mesh/vertices/uvs', skipping");
	} else {
		uvList = uvs->content.list;
	}

	GQueue *pList = positions->content.list;
	GQueue *tList = triangles->content.list;

	Mesh *mesh = createMesh(g_queue_get_length(pList), g_queue_get_length(tList));

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

				switch(pval->type) {
					case STORE_FLOAT_NUMBER:
						mesh->vertices[i].position[j] = pval->content.float_number;
					break;
					case STORE_INTEGER:
						mesh->vertices[i].position[j] = pval->content.integer;
					break;
					default:
						LOG_WARNING("Invalid vertex position value in component %d of vertex %d in mesh store, replacing by 0", j, i);
						mesh->vertices[i].position[j] = 0;
					break;
				}
			}
		}
	}

	if(colors != NULL) {
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

					switch(cval->type) {
						case STORE_FLOAT_NUMBER:
							mesh->vertices[i].color[j] = cval->content.float_number;
						break;
						case STORE_INTEGER:
							mesh->vertices[i].color[j] = cval->content.integer;
						break;
						default:
							LOG_WARNING("Invalid vertex color value in component %d of vertex %d in mesh store, replacing by 0", j, i);
							mesh->vertices[i].color[j] = 0;
						break;
					}
				}
			}
		}
	}

	if(uvs != NULL) {
		// Read UV coordinates
		i = 0;
		for(GList *iter = uvList->head; iter != NULL; iter = iter->next, i++) {
			Store *uv = iter->data;
			if(uv->type != STORE_LIST || g_queue_get_length(uv->content.list) != 2) {
				LOG_WARNING("Invalid UV coordinates for vertex %d in mesh store, replacing by 0/0", i);
				mesh->vertices[i].uv[0] = 0.0f;
				mesh->vertices[i].uv[1] = 0.0f;
			} else {
				int j = 0;
				for(GList *uviter = uv->content.list->head; uviter != NULL; uviter = uviter->next, j++) {
					Store *uvval = uviter->data;

					switch(uvval->type) {
						case STORE_FLOAT_NUMBER:
							mesh->vertices[i].uv[j] = uvval->content.float_number;
						break;
						case STORE_INTEGER:
							mesh->vertices[i].uv[j] = uvval->content.integer;
						break;
						default:
							LOG_WARNING("Invalid vertex UV coordinate value in component %d of vertex %d in mesh store, replacing by 0", j, i);
							mesh->vertices[i].uv[j] = 0;
						break;
					}
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

	generateMeshNormals(mesh);

	return mesh;
}

/**
 * Creates a store from a mesh
 *
 * @param mesh		the mesh to convert to a store
 * @result			the converted store or NULL on failure
 */
API Store *convertMeshToStore(Mesh *mesh)
{
	Store *store = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(store, "mesh", $(Store *, store, createStore)());
	$(bool, store, setStorePath)(store, "mesh/vertices", $(Store *, store, createStore)());

	Store *positions = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/vertices/positions", positions);
	Store *colors = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/vertices/colors", colors);
	Store *uvs = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/vertices/uvs", uvs);
	Store *triangles = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "mesh/triangles", triangles);

	// Write vertices
	for(int i = 0; i < mesh->num_vertices; i++) {
		Store *position = $(Store *, store, createStoreListValue)(NULL);
		Store *color = $(Store *, store, createStoreListValue)(NULL);
		Store *uv = $(Store *, store, createStoreListValue)(NULL);

		for(int j = 0; j < 4; j++) {
			if(j < 2) {
				g_queue_push_tail(uv->content.list, $(Store *, store, createStoreFloatNumberValue)(mesh->vertices[i].uv[j]));
			}
			if(j < 3) {
				g_queue_push_tail(position->content.list, $(Store *, store, createStoreFloatNumberValue)(mesh->vertices[i].position[j]));
			}
			g_queue_push_tail(color->content.list, $(Store *, store, createStoreFloatNumberValue)(mesh->vertices[i].color[j]));
		}

		g_queue_push_tail(positions->content.list, position);
		g_queue_push_tail(colors->content.list, color);
		g_queue_push_tail(uvs->content.list, uv);
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
