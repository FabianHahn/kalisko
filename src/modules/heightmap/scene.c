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

#include <assert.h>
#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/opengl/primitive.h"
#include "modules/image/io.h"
#include "modules/image/image.h"
#define API
#include "heightmap.h"
#include "scene.h"

API OpenGLPrimitive *parseOpenGLScenePrimitiveHeightmap(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse num parameter
	Store *heightmapParam;

	if((heightmapParam = getStorePath(store, "heightmap")) == NULL || heightmapParam->type != STORE_STRING) {
		LOG_ERROR("Failed to parse OpenGL scene primitive heightmap '%s': String parameter 'heightmap' not found", name);
		return NULL;
	}

	GString *path = g_string_new(path_prefix);
	g_string_append_printf(path, "/%s", heightmapParam->content.string);
	Image *image = readImageFromFile(path->str);
	g_string_free(path, true);

	if(image == NULL) {
		LOG_ERROR("Failed to parse OpenGL scene primitive heightmap '%s': Failed to load heightmap image from '%s'", name, path->str);
		return NULL;
	}

	// Create heightmap
	OpenGLPrimitive *primitive;

	if((primitive = createOpenGLPrimitiveHeightmap(image, image->width, image->height)) == NULL) {
		LOG_ERROR("Failed to parse OpenGL scene primitive heightmap '%s': Failed to create heightmap primitive from heightmap image", name);
		freeImage(image);
		return NULL;
	}

	return primitive;
}
