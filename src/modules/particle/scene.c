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
#include "modules/linalg/store.h"
#define API
#include "particle.h"
#include "scene.h"

API OpenGLPrimitive *parseOpenGLScenePrimitiveParticles(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	// Parse num parameter
	Store *numParam;

	if((numParam = $(Store *, store, getStorePath)(store, "num")) == NULL || numParam->type != STORE_INTEGER) {
		logError("Failed to parse OpenGL scene primitive particle effect '%s' - integer parameter 'num' not found", name);
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
		logInfo("Set lifetime for particle effect '%s'", name);
	}

	// Parse position mean parameter
	Store *positionMeanParam = $(Store *, store, getStorePath)(store, "positionMean");
	if(positionMeanParam != NULL && positionMeanParam->type == STORE_LIST) {
		Vector *positionMean = $(Vector *, linalg, convertStoreToVector)(positionMeanParam);
		$(void, linalg, assignVector)(particles->properties.positionMean, positionMean);
		$(void, linalg, freeVector)(positionMean);
		logInfo("Set mean position for particle effect '%s'", name);
	}

	// Parse position std parameter
	Store *positionStdParam = $(Store *, store, getStorePath)(store, "positionStd");
	if(positionStdParam != NULL && positionStdParam->type == STORE_LIST) {
		Vector *positionStd = $(Vector *, linalg, convertStoreToVector)(positionStdParam);
		$(void, linalg, assignVector)(particles->properties.positionStd, positionStd);
		$(void, linalg, freeVector)(positionStd);
		logInfo("Set position standard deviation for particle effect '%s'", name);
	}

	// Parse velocity mean parameter
	Store *velocityMeanParam = $(Store *, store, getStorePath)(store, "velocityMean");
	if(velocityMeanParam != NULL && velocityMeanParam->type == STORE_LIST) {
		Vector *velocityMean = $(Vector *, linalg, convertStoreToVector)(velocityMeanParam);
		$(void, linalg, assignVector)(particles->properties.velocityMean, velocityMean);
		$(void, linalg, freeVector)(velocityMean);
		logInfo("Set mean velocity for particle effect for particle effect '%s'", name);
	}

	// Parse velocity std parameter
	Store *velocityStdParam = $(Store *, store, getStorePath)(store, "velocityStd");
	if(velocityStdParam != NULL && velocityStdParam->type == STORE_LIST) {
		Vector *velocityStd = $(Vector *, linalg, convertStoreToVector)(velocityStdParam);
		$(void, linalg, assignVector)(particles->properties.velocityStd, velocityStd);
		$(void, linalg, freeVector)(velocityStd);
		logInfo("Set velocity standard deviation for particle effect '%s'", name);
	}

	// Parse start size param
	Store *startSizeParam;
	if((startSizeParam = $(Store *, store, getStorePath)(store, "startSize")) != NULL && (startSizeParam->type == STORE_INTEGER || startSizeParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.startSize = startSizeParam->type == STORE_FLOAT_NUMBER ? startSizeParam->content.float_number : startSizeParam->content.integer;
		logInfo("Set start size for particle effect '%s'", name);
	}

	// Parse end size param
	Store *endSizeParam;
	if((endSizeParam = $(Store *, store, getStorePath)(store, "startSize")) != NULL && (endSizeParam->type == STORE_INTEGER || endSizeParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.endSize = endSizeParam->type == STORE_FLOAT_NUMBER ? endSizeParam->content.float_number : endSizeParam->content.integer;
		logInfo("Set end size for particle effect '%s'", name);
	}

	// Parse aspect ratio param
	Store *aspectRatioParam;
	if((aspectRatioParam = $(Store *, store, getStorePath)(store, "aspectRatio")) != NULL && (aspectRatioParam->type == STORE_INTEGER || aspectRatioParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.aspectRatio = aspectRatioParam->type == STORE_FLOAT_NUMBER ? aspectRatioParam->content.float_number : aspectRatioParam->content.integer;
		logInfo("Set aspect ratio for particle effect '%s'", name);
	}

	// Parse angular velocity mean param
	Store *angularVelocityMeanParam;
	if((angularVelocityMeanParam = $(Store *, store, getStorePath)(store, "angularVelocityMean")) != NULL && (angularVelocityMeanParam->type == STORE_INTEGER || angularVelocityMeanParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.angularVelocityMean = angularVelocityMeanParam->type == STORE_FLOAT_NUMBER ? angularVelocityMeanParam->content.float_number : angularVelocityMeanParam->content.integer;
		logInfo("Set mean angular velocity for particle effect '%s'", name);
	}

	// Parse angular velocity std param
	Store *angularVelocityStdParam;
	if((angularVelocityStdParam = $(Store *, store, getStorePath)(store, "angularVelocityStd")) != NULL && (angularVelocityStdParam->type == STORE_INTEGER || angularVelocityStdParam->type == STORE_FLOAT_NUMBER)) {
		particles->properties.angularVelocityStd = angularVelocityStdParam->type == STORE_FLOAT_NUMBER ? angularVelocityStdParam->content.float_number : angularVelocityStdParam->content.integer;
		logInfo("Set standard deviation of angular velocity for particle effect '%s'", name);
	}

	initOpenGLPrimitiveParticles(primitive);

	return primitive;
}
