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
#include "modules/opengl/primitive.h"
#include "api.h"
#include "primitive.h"

/**
 * A table of strings associated with OpenGLPrimitiveSceneParser callbacks
 */
static GHashTable *parsers = NULL;

/**
 * Initializes the OpenGLPrimitive scene parsers
 */
API void initOpenGLPrimitiveSceneParsers()
{
	assert(parsers == NULL);
	parsers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
}

/**
 * Registers an OpenGLPrimitive scene parser
 *
 * @param type		the type that the OpenGLSceneParser is able to parse
 * @param parser	the parser callback to be registered
 * @result			true if successful
 */
API bool registerOpenGLPrimitiveSceneParser(const char *type, OpenGLPrimitiveSceneParser *parser)
{
	if(g_hash_table_lookup(parsers, type) != NULL) {
		LOG_ERROR("Tried to register OpenGLPrimitiveSceneParser for already registered type '%s'", type);
		return false;
	}

	g_hash_table_insert(parsers, strdup(type), parser);

	return true;
}

/**
 * Unregisters an OpenGLPrimitive scnee parser
 *
 * @param type		the type of the OpenGLSceneParser to be unregistered
 * @result			true if successful
 */
API bool unregisterOpenGLPrimitiveSceneParser(const char *type)
{
	return g_hash_table_remove(parsers, type);
}

/**
 * Parses an OpenGLPrimitive from a scene store by retrieving the correct registered parser for the type and executing it
 *
 * @param type		the type of the OpenGLPrimitive to parse
 * @param store		the store representation of the OpenGLPrimitive to parse
 * @result			the parsed OpenGLPrimitive or NULL on failure
 */
API OpenGLPrimitive *parseOpenGLScenePrimitive(const char *type, Store *store)
{
	OpenGLPrimitiveSceneParser *parser;

	if((parser = g_hash_table_lookup(parsers, type)) == NULL) {
		LOG_ERROR("Failed to parse OpenGLPrimitive from scene with type '%s' - no parser for that primitive type registered", type);
		return NULL;
	}

	// Execute the parser
	return parser(type, store);
}

/**
 * Frees the OpenGLPrimitive scene parsers
 */
API void freeOpenGLPrimitiveSceneParsers()
{
	assert(parsers != NULL);
	g_hash_table_destroy(parsers);
	parsers = NULL;
}
