/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "api.h"
#include "mesh.h"
#include "primitive.h"

/**
 * Creates an OpenGL primitive from a mesh
 *
 * @param mesh		the mesh to create a primitive from
 * @result			the created primitive
 */
API OpenGLPrimitive *createOpenGLPrimitiveMesh(OpenGLMesh *mesh)
{
	OpenGLPrimitive *primitive = ALLOCATE_OBJECT(OpenGLPrimitive);
	primitive->type = OPENGL_PRIMITIVE_MESH;
	primitive->value.mesh = mesh;

	return primitive;
}

/**
 * Draws an OpenGL primitive
 *
 * @param primitive			the primitive to draw
 */
API void drawOpenGLPrimitive(OpenGLPrimitive *primitive)
{
	switch(primitive->type) {
		case OPENGL_PRIMITIVE_MESH:
			drawOpenGLMesh(primitive->value.mesh);
		break;
		default:
			LOG_WARNING("Trying to draw unsupported OpenGL primitive type, skipping");
		break;
	}
}

/**
 * Frees an OpenGL primitive
 *
 * @param primitive		the primitive to free
 */
API void freeOpenGLPrimitive(OpenGLPrimitive *primitive)
{
	switch(primitive->type) {
		case OPENGL_PRIMITIVE_MESH:
			freeOpenGLMesh(primitive->value.mesh);
		break;
		default:
			LOG_WARNING("Trying to free unsupported OpenGL primitive type, skipping");
			return;
		break;
	}

	free(primitive);
}
