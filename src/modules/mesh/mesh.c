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

#include <assert.h>
#include <glib.h>
#include "dll.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "mesh.h"
#include "io.h"

MODULE_NAME("mesh");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a general mesh data type");
MODULE_VERSION(0, 5, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("linalg", 0, 2, 9));

MODULE_INIT
{
	initMeshIO();

	return true;
}

MODULE_FINALIZE
{
	freeMeshIO();
}

/**
 * Creates a new mesh by allocating space for a number of vertices and triangles
 *
 * @param num_vertices			the number of vertices the mesh should have
 * @param num_triangles			the number of triangles the mesh should have
 * @result						the created mesh object or NULL on failure
 */
Mesh *createMesh(int num_vertices, int num_triangles)
{
	assert(num_vertices > 0);
	assert(num_triangles > 0);

	Mesh *mesh = ALLOCATE_OBJECT(Mesh);
	mesh->vertices = ALLOCATE_OBJECTS(MeshVertex, num_vertices);
	mesh->num_vertices = num_vertices;
	mesh->triangles = ALLOCATE_OBJECTS(MeshTriangle, num_triangles);
	mesh->num_triangles = num_triangles;

	return mesh;
}

/**
 * Automatically generate normal vectors from triangles for a mesh
 *
 * @param mesh			the mesh to create normals for
 */
API void generateMeshNormals(Mesh *mesh)
{
	// Normalize normals
	for(int i = 0; i < mesh->num_vertices; i++) {
		// Reset normal vector
		mesh->vertices[i].normal[0] = 0.0f;
		mesh->vertices[i].normal[1] = 0.0f;
		mesh->vertices[i].normal[2] = 0.0f;
	}

	// Compute normals
	for(int i = 0; i < mesh->num_triangles; i++) {
		MeshVertex vertex1 = mesh->vertices[mesh->triangles[i].indices[0]];
		MeshVertex vertex2 = mesh->vertices[mesh->triangles[i].indices[1]];
		MeshVertex vertex3 = mesh->vertices[mesh->triangles[i].indices[2]];

		Vector *v1 = $(Vector *, linalg, createVector3)(vertex1.position[0], vertex1.position[1], vertex1.position[2]);
		Vector *v2 = $(Vector *, linalg, createVector3)(vertex2.position[0], vertex2.position[1], vertex2.position[2]);
		Vector *v3 = $(Vector *, linalg, createVector3)(vertex3.position[0], vertex3.position[1], vertex3.position[2]);
		Vector *e1 = $(Vector *, linalg, diffVectors)(v2, v1);
		Vector *e2 = $(Vector *, linalg, diffVectors)(v3, v1);
		Vector *normal = $(Vector *, linalg, crossVectors)(e1, e2);
		$(void, linalg, normalizeVector)(normal);
		float *normalData = $(float *, linalg, getVectorData)(normal);

		for(int j = 0; j < 3; j++) {
			int vi = mesh->triangles[i].indices[j];
			Vector *normi = $(Vector *, linalg, createVector3)(mesh->vertices[vi].normal[0], mesh->vertices[vi].normal[1], mesh->vertices[vi].normal[2]);

			if($(float, linalg, dotVectors)(normal, normi) >= 0) { // enforce interpolated normal orientation
				mesh->vertices[vi].normal[0] += normalData[0];
				mesh->vertices[vi].normal[1] += normalData[1];
				mesh->vertices[vi].normal[2] += normalData[2];
			} else {
				mesh->vertices[vi].normal[0] -= normalData[0];
				mesh->vertices[vi].normal[1] -= normalData[1];
				mesh->vertices[vi].normal[2] -= normalData[2];
			}

			$(void, linalg, freeVector)(normi);
		}

		$(void, linalg, freeVector)(v1);
		$(void, linalg, freeVector)(v2);
		$(void, linalg, freeVector)(v3);
		$(void, linalg, freeVector)(e1);
		$(void, linalg, freeVector)(e2);
		$(void, linalg, freeVector)(normal);
	}

	// Normalize normals
	for(int i = 0; i < mesh->num_vertices; i++) {
		MeshVertex vertex = mesh->vertices[i];
		Vector *v = $(Vector *, linalg, createVector3)(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
		$(void, linalg, normalizeVector)(v);
		float *vData = $(float *, linalg, getVectorData)(v);

		mesh->vertices[i].normal[0] = vData[0];
		mesh->vertices[i].normal[1] = vData[1];
		mesh->vertices[i].normal[2] = vData[2];

		$(void, linalg, freeVector)(v);
	}
}

/**
 * Frees a mesh
 *
 * @param mesh			the mesh to free
 */
void freeMesh(Mesh *mesh)
{
	assert(mesh != NULL);

	free(mesh->vertices);
	free(mesh->triangles);
	free(mesh);
}
