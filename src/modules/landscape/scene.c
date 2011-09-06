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
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/opengl/primitive.h"
#include "modules/image/io.h"
#include "modules/image/image.h"
#include "modules/heightmap/heightmap.h"
#define API
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

	// Parse worleyPoints parameter
	Store *worleyPointsParam;

	if((worleyPointsParam = $(Store *, store, getStorePath)(store, "worleyPoints")) == NULL || worleyPointsParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'worleyPoints' not found", name);
		return NULL;
	}

	unsigned int worleyPoints = worleyPointsParam->content.integer;

	// Parse fbmFrequency parameter
	Store *fbmFrequencyParam;

	if((fbmFrequencyParam = $(Store *, store, getStorePath)(store, "fbmFrequency")) == NULL || !(fbmFrequencyParam->type == STORE_FLOAT_NUMBER || fbmFrequencyParam->type == STORE_INTEGER)) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Float parameter 'fbmFrequency' not found", name);
		return NULL;
	}

	float fbmFrequency = fbmFrequencyParam->type == STORE_FLOAT_NUMBER ? fbmFrequencyParam->content.float_number : fbmFrequencyParam->content.integer;

	// Parse fbmPersistance parameter
	Store *fbmPersistanceParam;

	if((fbmPersistanceParam = $(Store *, store, getStorePath)(store, "fbmPersistance")) == NULL || !(fbmPersistanceParam->type == STORE_FLOAT_NUMBER || fbmPersistanceParam->type == STORE_INTEGER)) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Float parameter 'fbmPersistance' not found", name);
		return NULL;
	}

	float fbmPersistance = fbmPersistanceParam->type == STORE_FLOAT_NUMBER ? fbmPersistanceParam->content.float_number : fbmPersistanceParam->content.integer;

	// Parse fbmDepth parameter
	Store *fbmDepthParam;

	if((fbmDepthParam = $(Store *, store, getStorePath)(store, "fbmDepth")) == NULL || fbmDepthParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'fbmDepth' not found", name);
		return NULL;
	}

	unsigned int fbmDepth = fbmDepthParam->content.integer;

	// Parse erosionThermalIterations parameter
	Store *erosionThermalIterationsParam;

	if((erosionThermalIterationsParam = $(Store *, store, getStorePath)(store, "erosionThermalIterations")) == NULL || erosionThermalIterationsParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'erosionThermalIterations' not found", name);
		return NULL;
	}

	unsigned int erosionThermalIterations = erosionThermalIterationsParam->content.integer;

	// Parse erosionThermalTalusAngle parameter
	Store *erosionThermalTalusAngleParam;

	if((erosionThermalTalusAngleParam = $(Store *, store, getStorePath)(store, "erosionThermalTalusAngle")) == NULL || !(erosionThermalTalusAngleParam->type == STORE_FLOAT_NUMBER || erosionThermalTalusAngleParam->type == STORE_INTEGER)) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Float parameter 'erosionThermalTalusAngle' not found", name);
		return NULL;
	}

	float erosionThermalTalusAngle = erosionThermalTalusAngleParam->type == STORE_FLOAT_NUMBER ? erosionThermalTalusAngleParam->content.float_number : erosionThermalTalusAngleParam->content.integer;

	// Parse erosionHydraulicIterations parameter
	Store *erosionHydraulicIterationsParam;

	if((erosionHydraulicIterationsParam = $(Store *, store, getStorePath)(store, "erosionHydraulicIterations")) == NULL || erosionHydraulicIterationsParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Integer parameter 'erosionHydraulicIterations' not found", name);
		return NULL;
	}

	unsigned int erosionHydraulicIterations = erosionHydraulicIterationsParam->content.integer;

	// Generate heightmap
	Image *image = generateLandscapeHeightmap(width, height, worleyPoints, fbmFrequency, fbmPersistance, fbmDepth, erosionThermalIterations, erosionThermalTalusAngle, erosionHydraulicIterations);

	// Create heightmap
	OpenGLPrimitive *primitive;

	if((primitive = $(OpenGLPrimitive *, heightmap, createOpenGLPrimitiveHeightmap)(image, image->width, image->height)) == NULL) {
		LOG_ERROR("Failed to parse OpenGL scene primitive landscape '%s': Failed to create heightmap primitive from landscape heightmap image", name);
		$(Image *, image, freeImage)(image);
		return NULL;
	}

	return primitive;
}
