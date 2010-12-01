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

#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include <GL/glew.h>
#include <GL/freeglut.h>

/**
 * Struct representing an OpenGL vertex
 */
typedef struct {
	/** The position of the vertex */
	float position[3];
	/** The normal vector of the vertex */
	float normal[3];
	/** The color of the vertex */
	float color[4];
} OpenGLVertex;

/**
 * Struct representing an OpenGL triangle
 */
typedef struct {
	/** The vertex indices of the triangle */
	unsigned short indices[3];
} OpenGLTriangle;

/**
 * Struct representing an OpenGL triangle mesh
 */
typedef struct {
	/** An array of OpenGLVertex objects for this mesh */
	OpenGLVertex *vertices;
	/** The number of vertices in this mesh */
	int num_vertices;
	/** An array of OpenGLTriangle objects for this mesh */
	OpenGLTriangle *triangles;
	/** The number of triangles in this mesh */
	int num_triangles;
	/** The OpenGL vertex buffer associated with this mesh */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this mesh */
	GLuint indexBuffer;
	/** The OpenGL usage pattern of this mesh */
	GLenum usage;
} OpenGLMesh;

OpenGLMesh *createMesh(int num_vertices, int num_triangles, GLenum usage);
bool updateMesh(OpenGLMesh *mesh);
bool drawMesh(OpenGLMesh *mesh);
void freeMesh(OpenGLMesh *mesh);

#endif
