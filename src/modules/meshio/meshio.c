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
#include "api.h"
#include "meshio.h"

MODULE_NAME("meshio");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("I/O library for OpenGL meshes");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 10, 12));

/**
 * A hash table associating strings with MeshIOHandlers
 */
static GHashTable *handlers;

MODULE_INIT
{
	handlers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(handlers);
}

/**
 * Adds a mesh IO handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addMeshIOHandler(const char *extension, MeshIOHandler *handler)
{
	if(g_hash_table_lookup(handlers, extension) != NULL) {
		LOG_ERROR("Trying to add mesh IO handler for already handled extension '%s'", extension);
		return false;
	}

	g_hash_table_insert(handlers, strdup(extension), handler);
	return true;
}

/**
 * Removes a mesh IO handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteMeshIOHandler(const char *extension)
{
	return g_hash_table_remove(handlers, extension);
}

/**
 * Reads an OpenGL mesh from a file by using the appropriate handler
 *
 * @param filename			the mesh file that should be loaded
 * @result					the loaded mesh or NULL on error
 */
API OpenGLMesh *readMeshFromFile(const char *filename)
{

}
