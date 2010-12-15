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
#include "dll.h"
#include "modules/mesh/mesh.h"
#include "api.h"
#include "opengl.h"
#include "shader.h"
#include "material.h"
#include "mesh.h"

/**
 * Creates a new mesh by allocating space for a number of vertices and triangles. Also allocates needed OpenGL buffer objects
 *
 * @param mesh			the actual mesh geometry to use
 * @param usage			specifies the usage pattern of the mesh, see the OpenGL documentation on glBufferData() for details (if you don't know what this means, you can probably set it to GL_STATIC_DRAW)
 * @result				the created OpenGLMesh object or NULL on failure
 */
OpenGLMesh *createOpenGLMesh(Mesh *mesh, GLenum usage)
{
	OpenGLMesh *openglmesh = ALLOCATE_OBJECT(OpenGLMesh);
	openglmesh->mesh = mesh;
	openglmesh->usage = usage;

	glGenBuffers(1, &openglmesh->vertexBuffer);
	glGenBuffers(1, &openglmesh->indexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, openglmesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * mesh->num_vertices, NULL, usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openglmesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(MeshTriangle) * mesh->num_triangles, NULL, usage);

	if(checkOpenGLError()) {
		freeOpenGLMesh(openglmesh);
		return NULL;
	}

	return openglmesh;
}

/**
 * Updates a mesh by synchronizing it with its associated OpenGL buffer objects
 *
 * @param openglmesh			the mesh to be updated
 * @result						true if successful
 */
bool updateOpenGLMesh(OpenGLMesh *openglmesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, openglmesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * openglmesh->mesh->num_vertices, openglmesh->mesh->vertices, openglmesh->usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openglmesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(MeshTriangle) * openglmesh->mesh->num_triangles, openglmesh->mesh->triangles, openglmesh->usage);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Draws a mesh to OpenGL
 *
 * @param openglmesh			the mesh to draw
 */
bool drawOpenGLMesh(OpenGLMesh *openglmesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, openglmesh->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_VERTEX, 3, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_VERTEX);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_NORMAL, 3, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, normal));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_NORMAL);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_COLOR, 4, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, color));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_COLOR);

	if(checkOpenGLError()) {
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openglmesh->indexBuffer);
	glDrawElements(GL_TRIANGLES, openglmesh->mesh->num_triangles * 3, GL_UNSIGNED_SHORT, NULL);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Frees an OpenGL mesh
 *
 * @param openglmesh			the mesh to free
 */
void freeOpenGLMesh(OpenGLMesh *openglmesh)
{
	assert(openglmesh != NULL);

	glDeleteBuffers(1, &openglmesh->vertexBuffer);
	glDeleteBuffers(1, &openglmesh->indexBuffer);
	$(void, mesh, freeMesh)(openglmesh->mesh);
	free(openglmesh);
}
