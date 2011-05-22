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
#include "api.h"
#include "erosion.h"
#include "modules/image/io.h"

MODULE_NAME("erosion");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Erosion functions");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 6), MODULE_DEPENDENCY("image_pnm", 0, 1, 0), MODULE_DEPENDENCY("image_png", 0, 1, 2));

MODULE_INIT
{
	// test code, will be moved into a unit test
	Image *surface = $(Image *, image, readImageFromFile)("modules/erosion/erosion_in.png");
	assert(surface != NULL);
	erodeThermal(surface, M_PI/4.5, 50); // 40 deg
	assert($(bool, image, writeImageToFile)("modules/erosion/erosion_out.ppm", surface));

	return true;
}

MODULE_FINALIZE
{
}

static inline void erodeThermalCell(Image* hMap, unsigned int x, unsigned int y, float talusAngle)
{
	const float c = 0.5;
	const float cellSize = 0.001;
	float dMax = 0.0, dTotal = 0.0;
	float T = cellSize * tan(talusAngle);

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,hMap->height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,hMap->width)); j++) {
			if(j == x && i == y)
				continue;

			float di = getImage(hMap, x, y, 0) - getImage(hMap, j, i, 0);
			if(di > T) {
				dTotal += di;
				if(di > dMax) {
					dMax = di;
				}
			}
		}
	}

	if(!(dTotal > 0.0)) {
		return;
	}

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,hMap->height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,hMap->width)); j++) {
			if(j == x && i == y)
				continue;

			float di = getImage(hMap, x, y, 0) - getImage(hMap, j, i, 0);
			if(di > T) {
				float hNew = getImage(hMap, j, i, 0) + c * (dMax-T) * di/dTotal;
				setImage(hMap, j, i, 0, hNew);
				setImage(hMap, j, i, 1, hNew);
				setImage(hMap, j, i, 2, hNew);
			}
		}
	}
	// NOTE: The total amount of material is not preserved. Is this desirable?
/*
	float hNew = getImage(hMap, x, y, 0) - c * (dMax-T);
	setImage(hMap, x, y, 0, hNew);
	setImage(hMap, x, y, 1, hNew);
	setImage(hMap, x, y, 2, hNew);
*/
}

/**
 * Erodes the height map using thermal weathering.
 *
 * @param height_map	height map of the erosion surface
 * @param talusAngle	critical angle of response
 * @param steps			number of iteration steps
 */
API void erodeThermal(Image* heightMap, float talusAngle,  unsigned int steps)
{
	assert(heightMap != NULL);

	for(int k=0; k<steps; k++) {
		// erode one time step
		for(unsigned int y = 0; y < heightMap->height; y++) {
			for(unsigned int x = 0; x < heightMap->width; x++) {
				erodeThermalCell(heightMap, x, y, talusAngle);
			}
		}
	}
}

/**
 * Erodes the height map using hydraulic erosion.
 *
 * @param height_map	height map of the erosion surface
 * @param steps			number of iteration steps
 */
API void erodeHydraulic(Image* heightMap, unsigned int steps)
{
	assert(heightMap != NULL);

	Image *hMapIn, *hMapOut, *hMapCopy;

	// create a temporary copy
	hMapCopy = copyImage(heightMap, heightMap->type);
	assert(hMapCopy != NULL);

	hMapIn = heightMap;
	hMapOut = hMapCopy;

	for(int k=0; k<steps; k++) {
		// erode one time step
		for(unsigned int y = 0; y < heightMap->height; y++) {
			for(unsigned int x = 0; x < heightMap->width; x++) {
				// TODO
			}
		}
		// swap maps
		Image *hMapTmp = hMapIn;
		hMapIn = hMapOut;
		hMapOut = hMapTmp;
	}

	if(steps % 2 == 1) {
		// copy the result into the input image
		if(heightMap->type == IMAGE_TYPE_BYTE) {
			memcpy(heightMap->data.byte_data, hMapCopy->data.byte_data,
				   heightMap->width*heightMap->height*heightMap->channels*sizeof(unsigned char));
		} else if (heightMap->type == IMAGE_TYPE_FLOAT) {
			memcpy(heightMap->data.float_data, hMapCopy->data.float_data,
				   heightMap->width*heightMap->height*heightMap->channels*sizeof(float));
		} else {
			assert(0); // unhandled image type
		}
	}

	// cleanup
	freeImage(hMapCopy);
}
