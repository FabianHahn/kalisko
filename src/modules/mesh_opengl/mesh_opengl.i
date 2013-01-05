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

#ifndef MESH_OPENGL_MESH_OPENGL_H
#define MESH_OPENGL_MESH_OPENGL_H

#include <GL/glew.h>
#include "modules/mesh/mesh.h"
#include "modules/opengl/primitive.h"


/**
 * Creates a new OpenGL primitive from a mesh
 *
 * @param mesh			the actual mesh geometry to use
 * @param usage			specifies the usage pattern of the mesh, see the OpenGL documentation on glBufferData() for details (if you don't know what this means, you can probably set it to GL_STATIC_DRAW)
 * @result				the created OpenGL mesh primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveMesh(Mesh *mesh, GLenum usage);

/**
 * Synchronizing a primitive mesh with its associated OpenGL buffer objects
 *
 * @param primitive			the mesh primitive to be synchronized
 * @result					true if successful
 */
API bool synchronizeOpenGLPrimitiveMesh(OpenGLPrimitive *primitive);

/**
 * Draws an OpenGL mesh primitive
 *
 * @param primitive			the mesh primitive to draw
 * @param options_p			a pointer to custom options to be considered for this draw call
 */
API bool drawOpenGLPrimitiveMesh(OpenGLPrimitive *primitive, void *options_p);

/**
 * Frees an OpenGL mesh primitive
 *
 * @param primitive			the mesh primitive to free
 */
API void freeOpenGLPrimitiveMesh(OpenGLPrimitive *primitive);

#endif
