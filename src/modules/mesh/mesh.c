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
#include "api.h"
#include "mesh.h"

MODULE_NAME("mesh");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module providing a general mesh data type");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("linalg", 0, 2, 9));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

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
