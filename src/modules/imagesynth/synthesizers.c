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
#include "modules/random/perlin.h"
#include "modules/image/image.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/store.h"
#include "api.h"
#include "synthesizers.h"

/**
 * Synthesize an image using fBm noise
 *
 *  Store parameters:
 *  * float persistence		the persistence of the franctional Brownian noise to generate, should lie in (0,1) and specifies how much further octave levels contribute to the noise value
 *  * int depth				the number of octaves to overlay for the fractional Brownian noise
 *  * float frequencyX		the frequency in X direction to use for the underlying perlin noise
 *  * float frequencyY		the frequency in Y direction to use for the underlying perlin noise
 *  * vector colorLow		the low color to use for the noise image (dimensions must equal channel value)
 *  * vector colorHigh		the high color to use for the noise image (dimensions must equal channel value)
 *
 * @param width				the width of the image to synthesize
 * @param height			the height of the image to synthesize
 * @param channels			the number of channels of the image to synthesize
 * @param parameters		store representation of optional parameters for the synthesizer as described above
 * @result					the synthesized image or NULL on failure
 */
API Image *synthesizeImageFBm(unsigned int width, unsigned int height, unsigned int channels, Store *parameters)
{
	// parse persistence
	float persistence = 0.5f;
	Store *persistenceParam;
	if((persistenceParam = $(Store *, store, getStorePath)(parameters, "persistence")) != NULL && (persistenceParam->type == STORE_INTEGER || persistenceParam->type == STORE_FLOAT_NUMBER)) {
		persistence = persistenceParam->type == STORE_FLOAT_NUMBER ? persistenceParam->content.float_number : persistenceParam->content.integer;
	}

	// parse depth
	int depth = 4;
	Store *depthParam;
	if((depthParam = $(Store *, store, getStorePath)(parameters, "depth")) != NULL && depthParam->type == STORE_INTEGER) {
		depth = depthParam->content.integer;
	}

	// parse x frequency
	float frequencyX = 1.0f;
	Store *frequencyXParam;
	if((frequencyXParam = $(Store *, store, getStorePath)(parameters, "frequencyX")) != NULL && (frequencyXParam->type == STORE_INTEGER || frequencyXParam->type == STORE_FLOAT_NUMBER)) {
		frequencyX = frequencyXParam->type == STORE_FLOAT_NUMBER ? frequencyXParam->content.float_number : frequencyXParam->content.integer;
	}

	// parse y frequency
	float frequencyY = 1.0f;
	Store *frequencyYParam;
	if((frequencyYParam = $(Store *, store, getStorePath)(parameters, "frequencyY")) != NULL && (frequencyYParam->type == STORE_INTEGER || frequencyXParam->type == STORE_FLOAT_NUMBER)) {
		frequencyY = frequencyYParam->type == STORE_FLOAT_NUMBER ? frequencyYParam->content.float_number : frequencyYParam->content.integer;
	}

	// parse low color
	Vector *colorLow;
	Store *colorLowParam;
	if((colorLowParam = $(Store *, store, getStorePath)(parameters, "colorLow")) != NULL && colorLowParam->type == STORE_LIST) {
		colorLow = $(Vector *, linalg, convertStoreToVector)(colorLowParam);
	} else {
		colorLow = $(Vector *, linalg, createVector)(0);
	}

	// parse high color
	Vector *colorHigh;
	Store *colorHighParam;
	if((colorHighParam = $(Store *, store, getStorePath)(parameters, "colorHigh")) != NULL && colorHighParam->type == STORE_LIST) {
		colorHigh = $(Vector *, linalg, convertStoreToVector)(colorHighParam);
	} else {
		colorHigh = $(Vector *, linalg, createVector)(0);
	}

	// prepare data
	Image *image = $(Image *, image, createImageFloat)(width, height, channels);
	unsigned int lowSize = $(unsigned int, linalg, getVectorSize)(colorLow);
	float *lowData = $(float *, linalg, getVectorData)(colorLow);
	unsigned int highSize = $(unsigned int, linalg, getVectorSize)(colorHigh);
	float *highData = $(float *, linalg, getVectorData)(colorHigh);

	// generate fBm image
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			setImage(image, x, y, 0, $(float, random, noiseFBm)((double) y * frequencyY / height, (double) x * frequencyX / width, 0.0, persistence, depth));
		}
	}

	// normalize it
	$(void, image, normalizeImageChannel)(image, 0);

	// scale by low/high colors
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = getImage(image, x, y, 0);

			for(unsigned int c = 0; c < channels; c++) {
				float low = 0.0f;
				if(c < lowSize) {
					low = lowData[c];
				}

				float high = 1.0f;
				if(c < highSize) {
					high = highData[c];
				}

				setImage(image, x, y, c, low + (high - low) * value);
			}
		}
	}

	return image;
}
