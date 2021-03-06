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


#include <GL/glew.h>
#include <glib.h>
#include <assert.h>

#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/opengl/texture.h"
#define API
#include "texture.h"
#include "texture_parsers.h"

/**
 * A table of strings associated with OpenGLTextureSceneParser callbacks
 */
static GHashTable *parsers = NULL;

API void initOpenGLTextureSceneParsers()
{
	assert(parsers == NULL);
	parsers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	registerOpenGLTextureSceneParser("file", &parseOpenGLSceneTextureFile);
	registerOpenGLTextureSceneParser("array", &parseOpenGLSceneTextureArray);
}

API bool registerOpenGLTextureSceneParser(const char *type, OpenGLTextureSceneParser *parser)
{
	if(g_hash_table_lookup(parsers, type) != NULL) {
		logError("Tried to register OpenGLTextureSceneParser for already registered type '%s'", type);
		return false;
	}

	g_hash_table_insert(parsers, strdup(type), parser);

	logNotice("Registered parser for OpenGL scene texture type '%s'", type);

	return true;
}

API bool unregisterOpenGLTextureSceneParser(const char *type)
{
	return g_hash_table_remove(parsers, type);
}

API OpenGLTexture *parseOpenGLSceneTexture(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	assert(store->type == STORE_ARRAY);

	Store *type;

	if((type = $(Store *, store, getStorePath)(store, "type")) == NULL || type->type != STORE_STRING) {
		logError("Failed to parse OpenGL texture '%s' from scene - type parameter is not a string", name);
		return NULL;
	}

	OpenGLTextureSceneParser *parser;

	if((parser = g_hash_table_lookup(parsers, type->content.string)) == NULL) {
		logError("Failed to parse OpenGL texture '%s' from scene with type '%s' - no parser for that texture type registered", name, type->content.string);
		return NULL;
	}

	// Execute the parser
	return parser(scene, path_prefix, name, store);
}

API void freeOpenGLTextureSceneParsers()
{
	assert(parsers != NULL);
	g_hash_table_destroy(parsers);
	parsers = NULL;
}
