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
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "api.h"
#include "opengl.h"
#include "shader.h"
#include "material.h"
#include "mesh.h"

/**
 * Creates a new mesh by allocating space for a number of vertices and triangles. Also allocates needed OpenGL buffer objects
 *
 * @param num_vertices			the number of vertices the mesh should have
 * @param num_triangles			the number of triangles the mesh should have
 * @param usage					specifies the usage pattern of the mesh, see the OpenGL documentation on glBufferData() for details (if you don't know what this means, you can probably set it to GL_STATIC_DRAW)
 * @result						the created OpenGLMesh object or NULL on failure
 */
OpenGLMesh *createMesh(int num_vertices, int num_triangles, GLenum usage)
{
	assert(num_vertices > 0);
	assert(num_triangles > 0);

	OpenGLMesh *mesh = ALLOCATE_OBJECT(OpenGLMesh);
	mesh->vertices = ALLOCATE_OBJECTS(OpenGLVertex, num_vertices);
	mesh->num_vertices = num_vertices;
	mesh->triangles = ALLOCATE_OBJECTS(OpenGLTriangle, num_triangles);
	mesh->num_triangles = num_triangles;
	mesh->usage = usage;

	glGenBuffers(1, &mesh->vertexBuffer);
	glGenBuffers(1, &mesh->indexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(OpenGLVertex) * num_vertices, NULL, usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OpenGLTriangle) * num_triangles, NULL, usage);

	if(checkOpenGLError()) {
		freeMesh(mesh);
		return NULL;
	}

	return mesh;
}

/**
 * Updates a mesh by synchronizing it with its associated OpenGL buffer objects
 *
 * @param mesh			the mesh to be updated
 * @result				true if successful
 */
bool updateMesh(OpenGLMesh *mesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(OpenGLVertex) * mesh->num_vertices, mesh->vertices, mesh->usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(OpenGLTriangle) * mesh->num_triangles, mesh->triangles, mesh->usage);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Draws a mesh to OpenGL
 *
 * @param mesh			the mesh to draw
 */
bool drawMesh(OpenGLMesh *mesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_VERTEX, 3, GL_FLOAT, false, sizeof(OpenGLVertex), NULL + offsetof(OpenGLVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_VERTEX);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_NORMAL, 3, GL_FLOAT, false, sizeof(OpenGLVertex), NULL + offsetof(OpenGLVertex, normal));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_NORMAL);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_COLOR, 4, GL_FLOAT, false, sizeof(OpenGLVertex), NULL + offsetof(OpenGLVertex, color));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_COLOR);

	if(checkOpenGLError()) {
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
	glDrawElements(GL_TRIANGLES, mesh->num_triangles, GL_UNSIGNED_SHORT, NULL);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Frees an OpenGL mesh
 *
 * @param mesh			the mesh to free
 */
void freeMesh(OpenGLMesh *mesh)
{
	assert(mesh != NULL);

	glDeleteBuffers(1, &mesh->vertexBuffer);
	glDeleteBuffers(1, &mesh->indexBuffer);
	free(mesh->vertices);
	free(mesh->triangles);
	free(mesh);
}
