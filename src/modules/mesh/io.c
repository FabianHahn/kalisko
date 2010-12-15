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
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#include "api.h"
#include "mesh.h"
#include "io.h"
#include "store.h"

static Mesh *readMeshStore(const char *filename);
static bool writeMeshStore(const char *filename, Mesh *mesh);

/**
 * A hash table associating strings with MeshIOReadHandlers
 */
static GHashTable *readHandlers;

/**
 * A hash table associating strings with MeshIOWriteHandlers
 */
static GHashTable *writeHandlers;

/**
 * Initializes the mesh IO system
 */
API void initMeshIO()
{
	readHandlers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	writeHandlers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);

	addMeshIOReadHandler("store", &readMeshStore);
	addMeshIOWriteHandler("store", &writeMeshStore);
}

/**
 * Frees the mesh IO system
 */
API void freeMeshIO()
{
	g_hash_table_destroy(readHandlers);
	g_hash_table_destroy(writeHandlers);
}

/**
 * Adds a mesh IO reading handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addMeshIOReadHandler(const char *extension, MeshIOReadHandler *handler)
{
	if(g_hash_table_lookup(readHandlers, extension) != NULL) {
		LOG_ERROR("Trying to add mesh IO reading handler for already handled extension '%s'", extension);
		return false;
	}

	g_hash_table_insert(readHandlers, strdup(extension), handler);
	return true;
}

/**
 * Removes a mesh IO reading handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteMeshIOReadHandler(const char *extension)
{
	return g_hash_table_remove(readHandlers, extension);
}

/**
 * Reads a mesh from a file by using the appropriate handler
 *
 * @param filename			the mesh file that should be loaded
 * @result					the loaded mesh or NULL on error
 */
API Mesh *readMeshFromFile(const char *filename)
{
	if(!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		LOG_ERROR("Trying to read mesh from non existing file '%s'", filename);
		return NULL;
	}

	char *ext;

	if((ext = g_strrstr(filename, ".")) == NULL) {
		LOG_ERROR("Trying to read mesh from extensionless file '%s'", filename);
		return NULL;
	}

	ext++; // move past the dot

	MeshIOReadHandler *handler;
	if((handler = g_hash_table_lookup(readHandlers, ext)) == NULL) {
		LOG_ERROR("Tried to read mesh file '%s', but no handler was found for the extension '%s'", filename, ext);
		return NULL;
	}

	// We found a handler for this extension, so let it handle the reading
	return handler(filename);
}


/**
 * Adds a mesh IO writing handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addMeshIOWriteHandler(const char *extension, MeshIOWriteHandler *handler)
{
	if(g_hash_table_lookup(writeHandlers, extension) != NULL) {
		LOG_ERROR("Trying to add mesh IO writing handler for already handled extension '%s'", extension);
		return false;
	}

	g_hash_table_insert(writeHandlers, strdup(extension), handler);
	return true;
}

/**
 * Removes a mesh IO writing handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteMeshIOWriteHandler(const char *extension)
{
	return g_hash_table_remove(writeHandlers, extension);
}

/**
 * Writes an OpenGL mesh to a file by using the appropriate handler
 *
 * @param filename			the file into which the mesh should be written
 * @param mesh				the mesh to be written
 * @result					true if successful
 */
API bool writeMeshToFile(const char *filename, Mesh *mesh)
{
	char *ext;

	if((ext = g_strrstr(filename, ".")) == NULL) {
		LOG_ERROR("Trying to write mesh to extensionless file '%s'", filename);
		return false;
	}

	ext++; // move past the dot

	MeshIOWriteHandler *handler;
	if((handler = g_hash_table_lookup(writeHandlers, ext)) == NULL) {
		LOG_ERROR("Tried to write mesh to file '%s', but no handler was found for the extension '%s'", filename, ext);
		return false;
	}

	// We found a handler for this extension, so let it handle the reading
	return handler(filename, mesh);
}


/**
 * Reads a mesh from a store file
 *
 * @param filename			the store file to read from
 * @result					the parsed mesh of NULL on failure
 */
static Mesh *readMeshStore(const char *filename)
{
	Store *store;
	if((store = $(Store *, store, parseStoreFile)(filename)) == NULL) {
		LOG_ERROR("Failed to parse mesh store file '%s'", filename);
		return NULL;
	}

	Mesh *mesh = createMeshFromStore(store);

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
static bool writeMeshStore(const char *filename, Mesh *mesh)
{
	Store *store = convertMeshToStore(mesh);
	bool result = $(bool, store, writeStoreFile)(filename, store);
	$(void, store, freeStore)(store);

	return result;
}

