/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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

#ifndef MESH_IO_H
#define MESH_IO_H

#include "mesh.h"

typedef Mesh *(MeshIOReadHandler)(const char *filename);
typedef bool (MeshIOWriteHandler)(const char *filename, Mesh *mesh);


/**
 * Initializes the mesh IO system
 */
API void initMeshIO();

/**
 * Frees the mesh IO system
 */
API void freeMeshIO();

/**
 * Adds a mesh IO reading handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addMeshIOReadHandler(const char *extension, MeshIOReadHandler *handler);

/**
 * Removes a mesh IO reading handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteMeshIOReadHandler(const char *extension);

/**
 * Reads a mesh from a file by using the appropriate handler
 *
 * @param filename			the mesh file that should be loaded
 * @result					the loaded mesh or NULL on error
 */
API Mesh *readMeshFromFile(const char *filename);

/**
 * Adds a mesh IO writing handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addMeshIOWriteHandler(const char *extension, MeshIOWriteHandler *handler);

/**
 * Removes a mesh IO writing handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteMeshIOWriteHandler(const char *extension);

/**
 * Writes an OpenGL mesh to a file by using the appropriate handler
 *
 * @param filename			the file into which the mesh should be written
 * @param mesh				the mesh to be written
 * @result					true if successful
 */
API bool writeMeshToFile(const char *filename, Mesh *mesh);

#endif
