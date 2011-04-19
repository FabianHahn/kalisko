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
#include "api.h"
#include "particle.h"
#include "scene.h"

/**
 * Parses an OpenGL particle effect from a scene store
 *
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param store			the scene store to parse
 * @result				the parsed primitive or NULL on failure
 */
API OpenGLPrimitive *parseOpenGLScenePrimitiveParticles(const char *path_prefix, Store *store)
{
	// Parse num parameter
	Store *numParam;

	if((numParam = $(Store *, store, getStorePath)(store, "num")) == NULL || numParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive mesh - integer parameter 'num' not found");
		return NULL;
	}

	unsigned int num = numParam->content.integer;

	// Create particle effect
	OpenGLPrimitive *primitive;

	if((primitive = createOpenGLPrimitiveParticles(num)) == NULL) {
		return NULL;
	}

	OpenGLParticles *particles = $(OpenGLParticles *, particle, getOpenGLParticles)(primitive);

	// Parse lifetime parameter
	Store *lifetimeParam;

	if((lifetimeParam = $(Store *, store, getStorePath)(store, "lifetime")) != NULL && (lifetimeParam->type == STORE_INTEGER || lifetimeParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.lifetime = lifetimeParam->type == STORE_FLOAT_NUMBER ? lifetimeParam->content.float_number : lifetimeParam->content.integer;
	}

	initOpenGLPrimitiveParticles(primitive);

	return primitive;
}
