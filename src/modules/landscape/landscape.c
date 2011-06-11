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
#include <limits.h>
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
MODULE_VERSION(0, 1, 6);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("heightmap", 0, 2, 6), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("opengl", 0, 27, 0), MODULE_DEPENDENCY("scene", 0, 5, 2), MODULE_DEPENDENCY("image", 0, 5, 0), MODULE_DEPENDENCY("random", 0, 4, 1), MODULE_DEPENDENCY("erosion", 0, 1, 2), MODULE_DEPENDENCY("image_png", 0, 1, 2), MODULE_DEPENDENCY("image_pnm", 0, 2, 5));

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
	float maxValue = 0.0, minValue = FLT_MAX;

	map =		$(Image *, image, createImageFloat)(width, height, 1);
	worley = 	$(Image *, image, createImageFloat)(width, height, 1);
	fBm =		$(Image *, image, createImageFloat)(width, height, 1);

	// 1. create worley noise for the overall map structure (valleys, peaks and ridges)
	RandomWorleyCtx* worleyCtx = createWorleyCtx(20, 2);
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = $(float, random, randomWorley)(worleyCtx,
					(double) x / width, (double) y / height, 0.0,
					1, RANDOM_DIST_EUCLIDEAN);
			setImage(worley, x, y, 0, value);

			if(value > maxValue)
				maxValue = value;
			if(value < minValue)
				minValue = value;
		}
	}
	freeWorleyCtx(worleyCtx);

	// rescale heightmap to [0,1] and invert it
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = (getImage(worley, x, y, 0) - minValue) / (maxValue - minValue);
			setImage(worley, x, y, 0, 1.f - value);
		}
	}

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("worley.pgm", worley));
#endif
	maxValue = 0.0;
	minValue = 0.0;

	// 2. create fBm noise to get interresting features of different frequencies
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			float value = 0.5 + $(float, random, noiseFBm)(x, y, 0.0,
					frequency / width, frequency / height, 0.0,
					/* persistance */ 0.5,
					/* depth */ 6);
			setImage(fBm, x, y, 0, value);

			if(value > maxValue)
				maxValue = value;
			if(value < minValue)
				minValue = value;
		}
	}

	// rescale heightmap to [0,1]
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			setImage(fBm, x, y, 0, (getImage(fBm, x, y, 0) - minValue) / (maxValue - minValue));
		}
	}

#ifdef DEBUGIMAGES
	assert($(bool, image, writeImageToFile)("fbm.pgm", fBm));
#endif

	// 3. filter the worley noise to remove straight lines
	// TODO

	// 4. combine worley noise and fBm
	// TODO

	// 5. apply erosion to make the appearance physically-based
#if 0
	char *execpath = $$(char *, getExecutablePath)();
	GString *path = g_string_new(execpath);
	g_string_append(path, "/modules/erosion/erosion_in.png");
	Image *map = $(Image *, image, readImageFromFile)(path->str);
	assert(map != NULL);
	g_string_free(path, true);
	free(execpath);
#endif

	//erodeThermal(map, M_PI/4.5, 50); // 40 deg
	//erodeHydraulic(map, 100);


	// 6. profit!
	return fBm; // FIXME
}
