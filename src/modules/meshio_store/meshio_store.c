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

#include <glib.h>
#include "dll.h"
#include "modules/opengl/mesh.h"
#include "modules/meshio/meshio.h"
#include "modules/store/store.h"
#include "modules/mesh_store/mesh_store.h"
#include "api.h"

MODULE_NAME("meshio_store");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A module providing handlers for writing and reading OpenGL meshes in the store format");
MODULE_VERSION(0, 3, 0);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 10, 12), MODULE_DEPENDENCY("meshio", 0, 2, 0), MODULE_DEPENDENCY("store", 0, 6, 7), MODULE_DEPENDENCY("mesh_store", 0, 1, 0));

static OpenGLMesh *readOpenGLMeshStore(const char *filename);
static bool writeOpenGLMeshStore(const char *filename, OpenGLMesh *mesh);

MODULE_INIT
{
	if(!$(bool, meshio, addMeshIOReadHandler("store", &readOpenGLMeshStore))) {
		return false;
	}

	if(!$(bool, meshio, addMeshIOWriteHandler("store", &writeOpenGLMeshStore))) {
		// Delete read handler because it was already registered
		$(bool, meshio, deleteMeshIOReadHandler)("store");
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, meshio, deleteMeshIOReadHandler)("store");
	$(bool, meshio, deleteMeshIOWriteHandler)("store");
}

/**
 * Reads an OpenGL mesh from a store file
 *
 * @param filename			the store file to read from
 * @result					the parsed OpenGL mesh of NULL on failure
 */
static OpenGLMesh *readOpenGLMeshStore(const char *filename)
{
	Store *store;
	if((store = $(Store *, store, parseStoreFile)(filename)) == NULL) {
		LOG_ERROR("Failed to parse mesh store file '%s'", filename);
		return NULL;
	}

	OpenGLMesh *mesh = $(OpenGLMesh *, mesh_store, createOpenGLMeshFromStore)(store);

	$(void, store, freeStore)(store);

	return mesh;
}

/**
 * Writes an OpenGL mesh to a store file
 *
 * @param filename			the file name of the mesh store file to be saved
 * @param mesh				the OpenGL mesh to be written
 * @result					true if successful
 */
static bool writeOpenGLMeshStore(const char *filename, OpenGLMesh *mesh)
{
	Store *store = $(Store *, mesh_store, convertOpenGLMeshToStore)(mesh);
	bool result = $(bool, store, writeStoreFile)(filename, store);
	$(void, store, freeStore)(store);

	return result;
}
