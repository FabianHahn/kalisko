/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
#define API
#include "landscape.h"
#include "scene.h"

MODULE_NAME("landscape");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to display randomly generated landscapes");
MODULE_VERSION(0, 2, 12);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("heightmap", 0, 4, 4), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("scene", 0, 8, 0), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("random", 0, 6, 2), MODULE_DEPENDENCY("erosion", 0, 1, 2), MODULE_DEPENDENCY("image_pnm", 0, 2, 5));

MODULE_INIT
{
	return registerOpenGLPrimitiveSceneParser("landscape", &parseOpenGLScenePrimitiveLandscape);
}

MODULE_FINALIZE
{
	unregisterOpenGLPrimitiveSceneParser("landscape");
}

/**
 * Generates a random landscape heightmap from procedural noise
 *
 * A reasonable value for each parameter is provided in brackets.
 *
 * @param width							the width of the heightmap to generate
 * @param height						the height of the heightmap to generate
 * @param worleyPoints					number of worley points (16)
 * @param fbmFrequency					the frequency of the franctional Brownian noise (4)
 * @param fbmPersistance				the persistence of the franctional Brownian noise (0.5)
 * @param fbmDepth						the number of octaves to overlay for the fractional Brownian noise (6)
 * @param erosionThermalIterations		number of thermal iterations (50)
 * @param erosionThermalTalusAngle		the angle of response in degrees (40)
 * @param erosionHydraulicIterations	number of hydraulic iterations (80)
 * @return								the created landscape heightmap or NULL on error
 */
API Image *generateLandscapeHeightmap(unsigned int width, unsigned int height, unsigned int worleyPoints, double fbmFrequency, double fbmPersistance, unsigned int fbmDepth, unsigned int erosionThermalIterations, double erosionThermalTalusAngle, unsigned int erosionHydraulicIterations)
{
//#define DEBUGIMAGES
	Image *worley =	createImageFloat(width, height, 1);
	Image *fBm =	createImageFloat(width, height, 1);

	// internal parameters
	// blending ratio between worley noise and fBm
    const float mixRatio = 1/3.f;

    // convert the talus angle from degrees to radians
    erosionThermalTalusAngle = erosionThermalTalusAngle / 180.f * M_PI;

	// 1. create worley noise for the overall map structure (valleys, peaks and ridges)
	RandomWorleyContext* worleyContext = createWorleyContext(worleyPoints, 2);
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			Vector *point = createVector2((double) x / width, (double) y / height);
			float value = randomWorleyDifference21(worleyContext, point, RANDOM_WORLEY_DISTANCE_EUCLIDEAN);
			freeVector(point);

			setImage(worley, x, y, 0, value);
		}
	}
	freeWorleyContext(worleyContext);

	normalizeImageChannel(worley, 0);

#ifdef DEBUGIMAGES
	assert(writeImageToFile(worley, "01_worley.pgm"));
#endif
	// 2. create fBm noise to get interresting features of different frequencies
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = noiseFBm((double) x * fbmFrequency / width, (double) y * fbmFrequency / height, 0.0, fbmPersistance, fbmDepth);
			setImage(fBm, x, y, 0, value);
		}
	}

	normalizeImageChannel(fBm, 0);

#ifdef DEBUGIMAGES
	assert(writeImageToFile(fBm, "02_fbm.pgm"));
#endif

	// 3. combine worley noise and fBm
	Image *map = blendImages(worley, fBm, mixRatio);
	normalizeImageChannel(map, 0);


#ifdef DEBUGIMAGES
	assert(writeImageToFile(map, "03_mix.pgm"));
#endif

	// 4. apply a perturbation filter to remove straight lines
	// TODO

#ifdef DEBUGIMAGES
	//assert(writeImageToFile("04_perturbed.pgm", map));
#endif

	// 5. apply erosion to make the appearance physically-based
    erodeThermal(map, erosionThermalTalusAngle, erosionThermalIterations);
    erodeHydraulic(map, erosionHydraulicIterations);

#ifdef DEBUGIMAGES
	assert(writeImageToFile(map, "05_erosion.pgm"));
#endif

	// 6. profit!
	freeImage(worley);
	freeImage(fBm);

	return map;
}
