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
#include "modules/image/image.h"
#include "modules/image/io.h"
#define API
#include "texture.h"
#include "texture_parsers.h"

/**
 * OpenGLTextureScene parser for 2D textures read from files
 *
 * @param scene			the scene to parse the OpenGL texture for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse
 * @param store			the store representation of the OpenGLTexture to parse
 * @result				the parsed OpenGLTexture or NULL on failure
 */
API OpenGLTexture *parseOpenGLSceneTextureFile(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse filename parameter
	Store *filenameParam;
	if((filenameParam = $(Store *, store, getStorePath)(store, "filename")) == NULL || filenameParam->type != STORE_STRING) {
		LOG_ERROR("Failed to parse OpenGL scene texture file '%s' - string parameter 'filename' not found", name);
		return NULL;
	}

	GString *filename = g_string_new(path_prefix);
	g_string_append(filename, filenameParam->content.string);
	Image *image = $(Image *, image, readImageFromFile)(filename->str);
	g_string_free(filename, true);

	if(image == NULL) {
		LOG_ERROR("Failed to read image file from '%s' for texture '%s'", filename->str, name);
		return NULL;
	}

	OpenGLTexture *texture;
	if((texture = $(OpenGLTexture *, opengl, createOpenGLTexture2D)(image, true)) == NULL) {
		LOG_ERROR("Failed to create OpenGL texture '%s' for scene", name);
		$(void, image, freeImage)(image);
		return NULL;
	}

	return texture;
}

/**
 * OpenGLTextureScene parser for texture arrays
 *
 * @param scene			the scene to parse the OpenGL texture for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse
 * @param store			the store representation of the OpenGLTexture to parse
 * @result				the parsed OpenGLTexture or NULL on failure
 */
API OpenGLTexture *parseOpenGLSceneTextureArray(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse textures parameter
	Store *texturesParam;
	if((texturesParam = $(Store *, store, getStorePath)(store, "textures")) == NULL || texturesParam->type != STORE_LIST) {
		LOG_ERROR("Failed to parse OpenGL scene texture array '%s' - list parameter 'textures' not found", name);
		return NULL;
	}

	// read images
	GPtrArray *images = g_ptr_array_new();
	for(GList *iter = texturesParam->content.list->head; iter != NULL; iter = iter->next) {
		Store *value = iter->data;
		if(value->type == STORE_ARRAY) {
			OpenGLTexture *texture;
			if((texture = parseOpenGLSceneTexture(scene, path_prefix, name, value)) != NULL) {
				if(texture->type == OPENGL_TEXTURE_TYPE_2D) {
					Image *textureImage = $(Image *, image, copyImage)(texture->image, texture->image->type); // clone image
					g_ptr_array_add(images, textureImage);
				} else {
					LOG_WARNING("Failed to parse element %d for scene texture array: parsed texture is not a 2D texture", images->len);
				}

				$(void, opengl, freeOpenGLTexture)(texture);
			}
		} else {
			LOG_WARNING("Failed to parse element %d for scene texture array: list element is not an array", images->len);
		}
	}

	// create opengl texture array
	OpenGLTexture *texture = $(OpenGLTexture *, opengl, createOpenGLTexture2DArray)((Image **) images->pdata, images->len, true);

	// free images
	for(unsigned int i = 0; i < images->len; i++) {
		$(void, image, freeImage)(images->pdata[i]);
	}
	g_ptr_array_free(images, true);

	return texture;
}
