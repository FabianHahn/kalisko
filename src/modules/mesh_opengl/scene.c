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
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/mesh/mesh.h"
#include "modules/mesh/io.h"
#include "modules/opengl/primitive.h"
#include "api.h"
#include "mesh_opengl.h"
#include "scene.h"

/**
 * Parses an OpenGL primitive mesh from a scene store
 *
 * @param scene			the scene to parse the OpenGL primitive for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse*
 * @param store			the scene store to parse
 * @result				the parsed primitive or NULL on failure
 */
API OpenGLPrimitive *parseOpenGLScenePrimitiveMesh(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse filename parameter
	Store *filenameParam;

	if((filenameParam = $(Store *, store, getStorePath)(store, "filename")) == NULL || filenameParam->type != STORE_STRING) {
		LOG_ERROR("Failed to parse OpenGL scene primitive mesh '%s' - string parameter 'filename' not found", name);
		return NULL;
	}

	GString *filename = g_string_new(path_prefix);
	g_string_append(filename, filenameParam->content.string);

	// Parse usage parameter
	Store *usageParam;
	GLenum usage;

	if((usageParam = $(Store *, store, getStorePath)(store, "usage")) == NULL || usageParam->type != STORE_STRING) {
		LOG_DEBUG("OpenGL scene primitive mesh '%s' 'usage' parameter not specified, defaulting to GL_STATIC_DRAW", name);
		usage = GL_STATIC_DRAW;
	} else {
		if(g_strcmp0(usageParam->content.string, "GL_STREAM_DRAW") == 0) {
			usage = GL_STREAM_DRAW;
		} else if(g_strcmp0(usageParam->content.string, "GL_STATIC_DRAW") == 0) {
			usage = GL_STATIC_DRAW;
		} else if(g_strcmp0(usageParam->content.string, "GL_DYNAMIC_DRAW") == 0) {
			usage = GL_DYNAMIC_DRAW;
		} else {
			LOG_WARNING("Invalid OpenGL scene primitive mesh '%s' 'usage' parameter specified, defaulting to GL_STATIC_DRAW", name);
			usage = GL_STATIC_DRAW;
		}
	}

	// Create mesh
	Mesh *mesh = $(Mesh *, mesh, readMeshFromFile)(filename->str);
	g_string_free(filename, true);

	if(mesh == NULL) {
		return NULL;
	}

	// Create primitive
	OpenGLPrimitive *primitive;

	if((primitive = createOpenGLPrimitiveMesh(mesh, usage)) == NULL) {
		return NULL;
	}

	return primitive;
}
