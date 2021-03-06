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

#ifndef SCENE_TEXTURE_H
#define SCENE_TEXTURE_H

#include <glib.h>
#include <GL/glew.h>
#include "modules/store/store.h"
#include "modules/opengl/texture.h"
#include "scene.h"

/**
 * Function pointer type for an OpenGLTexture scene parser
 */
typedef OpenGLTexture *(OpenGLTextureSceneParser)(Scene *scene, const char *path_prefix, const char *name, Store *store);


/**
 * Initializes the OpenGLPrimitive texture parsers
 */
API void initOpenGLTextureSceneParsers();

/**
 * Registers an OpenGLTexture scene parser
 *
 * @param type		the type that the OpenGLTextureSceneParser is able to parse
 * @param parser	the parser callback to be registered
 * @result			true if successful
 */
API bool registerOpenGLTextureSceneParser(const char *type, OpenGLTextureSceneParser *parser);

/**
 * Unregisters an OpenGLTexture scene parser
 *
 * @param type		the type of the OpenGLSceneParser to be unregistered
 * @result			true if successful
 */
API bool unregisterOpenGLTextureSceneParser(const char *type);

/**
 * Parses an OpenGL texture from a scene store by retrieving the correct registered parser for the type and executing it
 *
 * @param scene			the scene to parse the OpenGL texture for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse
 * @param store			the store representation of the OpenGLTexture to parse
 * @result				the parsed OpenGLTexture or NULL on failure
 */
API OpenGLTexture *parseOpenGLSceneTexture(Scene *scene, const char *path_prefix, const char *name, Store *store);

/**
 * Frees the OpenGLTexture scene parsers
 */
API void freeOpenGLTextureSceneParsers();

#endif
