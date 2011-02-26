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
 * Struct representing an OpenGL triangle mesh
 */
typedef struct {
	/** The actual mesh geometry to render */
	Mesh *mesh;
	/** The OpenGL vertex buffer associated with this mesh */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this mesh */
	GLuint indexBuffer;
	/** The OpenGL usage pattern of this mesh */
	GLenum usage;
	/** The OpenGL primitive used to render the mesh */
	OpenGLPrimitive primitive;
} OpenGLMesh;

/**
 * Creates a new OpenGL primitive from a mesh
 *
 * @param mesh			the actual mesh geometry to use
 * @param usage			specifies the usage pattern of the mesh, see the OpenGL documentation on glBufferData() for details (if you don't know what this means, you can probably set it to GL_STATIC_DRAW)
 * @result				the created OpenGLMesh object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveMesh(Mesh *mesh, GLenum usage)
{
	OpenGLMesh *openglmesh = ALLOCATE_OBJECT(OpenGLMesh);
	openglmesh->mesh = mesh;
	openglmesh->usage = usage;
	openglmesh->primitive.type = "mesh";
	openglmesh->primitive.data = openglmesh;
	openglmesh->primitive.draw_function = &drawOpenGLPrimitiveMesh;
	openglmesh->primitive.free_function = &freeOpenGLPrimitiveMesh;

	glGenBuffers(1, &openglmesh->vertexBuffer);
	glGenBuffers(1, &openglmesh->indexBuffer);
	updateOpenGLPrimitiveMesh(&openglmesh->primitive);

	if(checkOpenGLError()) {
		freeOpenGLPrimitiveMesh(&openglmesh->primitive);
		return NULL;
	}

	return &openglmesh->primitive;
}

/**
 * Updates a mesh primitive by synchronizing it with its associated OpenGL buffer objects
 *
 * @param primitive			the mesh primitive to be updated
 * @result					true if successful
 */
API bool updateOpenGLPrimitiveMesh(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "mesh") != 0) {
		LOG_ERROR("Failed to update OpenGL primitive mesh: Primitive is not a mesh");
		return false;
	}

	OpenGLMesh *openglmesh = primitive->data;

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
 * Draws an OpenGL mesh primitive
 *
 * @param primitive			the mesh primitive to draw
 */
API bool drawOpenGLPrimitiveMesh(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "mesh") != 0) {
		LOG_ERROR("Failed to update OpenGL primitive mesh: Primitive is not a mesh");
		return false;
	}

	OpenGLMesh *openglmesh = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, openglmesh->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_VERTEX, 3, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_VERTEX);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_NORMAL, 3, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, normal));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_NORMAL);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_COLOR, 4, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, color));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_COLOR);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_UV, 2, GL_FLOAT, false, sizeof(MeshVertex), NULL + offsetof(MeshVertex, uv));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_UV);

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
 * Frees an OpenGL mesh primitive
 *
 * @param primitive			the mesh primitive to free
 */
API void freeOpenGLPrimitiveMesh(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "mesh") != 0) {
		LOG_ERROR("Failed to update OpenGL primitive mesh: Primitive is not a mesh");
		return;
	}

	OpenGLMesh *openglmesh = primitive->data;

	glDeleteBuffers(1, &openglmesh->vertexBuffer);
	glDeleteBuffers(1, &openglmesh->indexBuffer);
	$(void, mesh, freeMesh)(openglmesh->mesh);
	free(openglmesh);
}
