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

#include <assert.h>
#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/opengl/primitive.h"
#include "modules/image/io.h"
#include "modules/image/image.h"
#include "modules/heightmap/heightmap.h"
#include "api.h"
#include "landscape.h"
#include "scene.h"

/**
 * Parses a landscape from a scene store
 *
 * @param scene			the scene to parse the OpenGL primitive for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse
 * @param store			the scene store to parse
 * @result				the parsed primitive or NULL on failure
 */
API OpenGLPrimitive *parseOpenGLScenePrimitiveLandscape(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse width parameter
	Store *widthParam;

	if((widthParam = $(Store *, store, getStorePath)(store, "width")) == NULL || widthParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'width' not found", name);
		return NULL;
	}

	unsigned int width = widthParam->content.integer;

	// Parse height parameter
	Store *heightParam;

	if((heightParam = $(Store *, store, getStorePath)(store, "height")) == NULL || heightParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'height' not found", name);
		return NULL;
	}

	unsigned int height = heightParam->content.integer;

	// Parse frequency parameter
	Store *frequencyParam;

	if((frequencyParam = $(Store *, store, getStorePath)(store, "frequency")) == NULL || !(frequencyParam->type == STORE_FLOAT_NUMBER || frequencyParam->type == STORE_INTEGER)) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Float parameter 'frequency' not found", name);
		return NULL;
	}

	double frequency = frequencyParam->type == STORE_FLOAT_NUMBER ? frequencyParam->content.float_number : frequencyParam->content.integer;

	// Generate heightmap
	Image *image = generateLandscapeHeightmap(width, height, frequency);

	// Create heightmap
	OpenGLPrimitive *primitive;

	if((primitive = $(OpenGLPrimitive *, heightmap, createOpenGLPrimitiveHeightmap)(image)) == NULL) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Failed to create heightmap primitive from landscape heightmap image", name);
		$(Image *, image, freeImage)(image);
		return NULL;
	}

	return primitive;
}