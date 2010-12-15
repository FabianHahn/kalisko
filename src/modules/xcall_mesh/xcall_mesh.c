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

#include <glib.h>

#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/merge.h"
#include "modules/xcall/xcall.h"
#include "modules/meshio/meshio.h"
#include "modules/mesh_store/mesh_store.h"
#include "modules/opengl/opengl.h"
#include "api.h"

static Store *xcall_readOpenGLMeshFile(Store *xcall);
static Store *xcall_writeOpenGLMeshFile(Store *xcall);

MODULE_NAME("xcall_mesh");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for meshes");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 10, 12), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("meshio", 0, 2, 0), MODULE_DEPENDENCY("mesh_store", 0, 1, 1), MODULE_DEPENDENCY("xcall", 0, 2, 6));

MODULE_INIT
{
	bool fail = true;

	do {
		if(!$(bool, xcall, addXCallFunction)("readOpenGLMeshFile", &xcall_readOpenGLMeshFile)) {
			break;
		}

		if(!$(bool, xcall, addXCallFunction)("writeOpenGLMeshFile", &xcall_writeOpenGLMeshFile)) {
			break;
		}

		fail = false;
	}
	while(false);

	if(fail) {
		$(bool, xcall, delXCallFunction)("readOpenGLMeshFile");
		$(bool, xcall, delXCallFunction)("writeOpenGLMeshFile");
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("readOpenGLMeshFile");
	$(bool, xcall, delXCallFunction)("writeOpenGLMeshFile");
}

/**
 * XCallFunction to read an OpenGL mesh from a file
 * XCall parameters:
 *  * string file			the filename of the mesh to read
 * XCall result:
 * 	* array mesh			the parsed mesh
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_readOpenGLMeshFile(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());

	Store *file;
	if((file = $(Store *, store, getStorePath)(xcall, "file")) == NULL || file->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'file'"));
		return ret;
	}

	OpenGLMesh *mesh;
	if((mesh = $(OpenGLMesh *, meshio, readMeshFromFile)(file->content.string)) == NULL) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read OpenGL mesh from specified file"));
		return ret;
	}

	Store *meshstore;
	if((meshstore = $(Store *, mesh_store, convertOpenGLMeshToStore)(mesh)) == NULL) {
		$(void, opengl, freeOpenGLMesh)(mesh);
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to convert OpenGL mesh to store"));
		return ret;
	}

	$(bool, store, mergeStore)(ret, meshstore);
	$(void, store, freeStore)(meshstore);

	return ret;
}

/**
 * XCallFunction to write an OpenGL mesh to a file
 * XCall parameters:
 *  * string file 		the filename of the mesh to write
 *  * array mesh		the mesh to write to the file
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_writeOpenGLMeshFile(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());

	Store *file;
	if((file = $(Store *, store, getStorePath)(xcall, "file")) == NULL || file->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'file'"));
		return ret;
	}

	Store *meshstore;
	if((meshstore = $(Store *, store, getStorePath)(xcall, "mesh")) == NULL || meshstore->type != STORE_ARRAY) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory array parameter 'mesh'"));
		return ret;
	}

	OpenGLMesh *mesh;
	if((mesh = $(OpenGLMesh *, mesh_store, createOpenGLMeshFromStore)(xcall)) == NULL) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to create OpenGL mesh from store"));
		return ret;
	}

	$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)($(bool, meshio, writeMeshToFile)(file->content.string, mesh)));
	$(void, opengl, freeOpenGLMesh)(mesh);

	return ret;
}
