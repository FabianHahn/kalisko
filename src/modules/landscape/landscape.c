/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *	 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *	 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *	   in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <assert.h>
#include <glib.h>
#include <stdio.h>
#include "dll.h"
#include "modules/erosion/erosion.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#include "modules/random/perlin.h"
#include "modules/random/worley.h"
#include "modules/scene/primitive.h"
#include "api.h"
#include "landscape.h"
#include "scene.h"

MODULE_NAME("landscape");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to display randomly generated landscapes");
MODULE_VERSION(0, 1, 10);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("heightmap", 0, 2, 6), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("opengl", 0, 27, 0), MODULE_DEPENDENCY("scene", 0, 5, 2), MODULE_DEPENDENCY("image", 0, 5, 9), MODULE_DEPENDENCY("random", 0, 6, 2), MODULE_DEPENDENCY("erosion", 0, 1, 2), MODULE_DEPENDENCY("image_pnm", 0, 2, 5));

MODULE_INIT
{
	return $(bool, scene, registerOpenGLPrimitiveSceneParser)("landscape", &parseOpenGLScenePrimitiveLandscape);
}

MODULE_FINALIZE
{
	$(bool, scene, unregisterOpenGLPrimitiveSceneParser)("landscape");
}

/**
 * Generates a random landscape heightmap from procedural noise
 *
 * @param width			the width of the heightmap to generate
 * @param height		the height of the heightmap to generate
 * @param frequency		the frequency to be used for the noise function
 * @return				the created landscape heightmap or NULL on error
 */
API Image *generateLandscapeHeightmap(unsigned int width, unsigned int height, double frequency)
{
#define DEBUGIMAGES
	// inefficent but useful for debugging
	Image *map, *worley, *fBm;
	map =		$(Image *, image, createImageFloat)(width, height, 1);
	worley = 	$(Image *, image, createImageFloat)(width, height, 1);
	fBm =		$(Image *, image, createImageFloat)(width, height, 1);

    // possible parameters (we should not expose all of them!)
    const unsigned int worleyPoints = 16;
    const float fbmPersistance = 0.5;
    const unsigned int fbmDepth = 6;
    const float mixRatio = 1/3.f; // internal
    const unsigned int erosionThermalIterations = 0; // 50
    const float erosionThermalTalusAngle = M_PI/4.5; // 40 deg
    const unsigned int erosionHydraulicIterations = 80; // 100
    // TODO: expose more hydraulic erosion parameters if needed

	// 1. create worley noise for the overall map structure (valleys, peaks and ridges)
	RandomWorleyContext* worleyContext = $(RandomWorleyContext *, random, createWorleyContext)(worleyPoints, 2);
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			Vector *point = $(Vector *, linalg, createVector2)((double) x / width, (double) y / height);
			float value = $(float, random, randomWorleyDifference21)(worleyContext, point, RANDOM_WORLEY_DISTANCE_EUCLIDEAN);
			$(void, linalg, freeVector)(point);

			setImage(worley, x, y, 0, value);
		}
	}
	$(void, random, freeWorleyContext)(worleyContext);

	$(void, image, normalizeImageChannel)(worley, 0);

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("01_worley.pgm", worley));
#endif
	// 2. create fBm noise to get interresting features of different frequencies
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = $(float, random, noiseFBm)((double) x * frequency / width, (double) y * frequency / height, 0.0, fbmPersistance, fbmDepth);
			setImage(fBm, x, y, 0, value);
		}
	}

	$(void, image, normalizeImageChannel)(fBm, 0);

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("02_fbm.pgm", fBm));
#endif

	// 3. combine worley noise and fBm
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = mixRatio * getImage(worley, x, y, 0) + (1.f - mixRatio) * getImage(fBm, x, y, 0);
			setImage(map, x, y, 0, value);
		}
	}

	$(void, image, normalizeImageChannel)(map, 0);

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("03_mix.pgm", map));
#endif

	// 4. apply a perturbation filter to remove straight lines
	// TODO

	// 5. apply erosion to make the appearance physically-based
    erodeThermal(map, erosionThermalTalusAngle, erosionThermalIterations);
    erodeHydraulic(map, erosionHydraulicIterations);

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("05_erosion.pgm", map));
#endif

	// 6. profit!
	$(void, image, freeImage)(worley);
	$(void, image, freeImage)(fBm);

	return map;
}
