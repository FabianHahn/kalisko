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

#ifndef MESH_MESH_H
#define MESH_MESH_H

/**
 * Struct representing a vertex
 */
typedef struct {
	/** The position of the vertex */
	float position[3];
	/** The normal vector of the vertex */
	float normal[3];
	/** The color of the vertex */
	float color[4];
	/** The uv coordinates of the vertex */
	float uv[2];
} MeshVertex;

/**
 * Struct representing a triangle
 */
typedef struct {
	/** The vertex indices of the triangle */
	unsigned short indices[3];
} MeshTriangle;

/**
 * Struct representing an OpenGL triangle mesh
 */
typedef struct {
	/** An array of MeshVertex objects for this mesh */
	MeshVertex *vertices;
	/** The number of vertices in this mesh */
	int num_vertices;
	/** An array of MeshTriangle objects for this mesh */
	MeshTriangle *triangles;
	/** The number of triangles in this mesh */
	int num_triangles;
} Mesh;

API Mesh *createMesh(int num_vertices, int num_triangles);
API void generateMeshNormals(Mesh *mesh);
API void freeMesh(Mesh *mesh);

#endif
