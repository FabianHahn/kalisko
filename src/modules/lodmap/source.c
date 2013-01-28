/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
#define API
#include "lodmap.h"
#include "source.h"
#include "imagesource.h"
#include "importsource.h"

/**
 * Hashtable associating string types with their associated OpenGLLodMapDataSourceFactory objects
 */
static GHashTable *factories;

API void initOpenGLLodMapDataSourceFactories()
{
	factories = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	registerOpenGLLodMapDataSourceFactory("image", &createOpenGLLodMapImageSourceFromStore);
	registerOpenGLLodMapDataSourceFactory("import", &createOpenGLLodMapImportSourceFromStore);
}

API void freeOpenGLLodMapDataSourceFactories()
{
	unregisterOpenGLLodMapDataSourceFactory("image");
	unregisterOpenGLLodMapDataSourceFactory("import");
	g_hash_table_destroy(factories);
}

API OpenGLLodMapDataSource *createOpenGLLodMapDataSourceFromStore(Store *store)
{
	Store *paramType = getStorePath(store, "lodmap/source/type");
	if(paramType == NULL || paramType->type != STORE_STRING) {
		logError("Failed to create OpenGL LOD map data source factory: Config string value 'lodmap/source/type' not found!");
		return NULL;
	}

	char *type = paramType->content.string;

	OpenGLLodMapDataSourceFactory *factory = g_hash_table_lookup(factories, type);
	if(factory == NULL) {
		logError("Failed to create OpenGL LOD map data source factory for type '%s': A factory for that type already exists!", type);
		return NULL;
	}

	// We got a working factory, so pass it the store configuration and run it...
	return factory(store);
}

API bool registerOpenGLLodMapDataSourceFactory(const char *type, OpenGLLodMapDataSourceFactory *factory)
{
	if(g_hash_table_lookup(factories, type) != NULL) {
		logError("Failed to register OpenGL LOD map data source factory for type '%s': A factory for that type already exists!", type);
		return false;
	}

	g_hash_table_insert(factories, strdup(type), factory);
	return true;
}

API bool unregisterOpenGLLodMapDataSourceFactory(const char *type)
{
	return g_hash_table_remove(factories, type);
}
