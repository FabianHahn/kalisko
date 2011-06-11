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
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 13), MODULE_DEPENDENCY("image_pnm", 0, 1, 0), MODULE_DEPENDENCY("image_png", 0, 1, 2));

MODULE_INIT
{
#if 0
	// test code, will be moved into a unit test
	char *execpath = $$(char *, getExecutablePath)();
	GString *path = g_string_new(execpath);
	g_string_append(path, "/modules/erosion/erosion_in.png");
	Image *surface1 = $(Image *, image, readImageFromFile)(path->str);
	assert(surface1 != NULL);
	Image *surface2 = copyImage(surface1, surface1->type);
	assert(surface2 != NULL);
	g_string_free(path, true);

	erodeThermal(surface1, M_PI/4.5, 50); // 40 deg
	erodeHydraulic(surface2, 100);

	path = g_string_new(execpath);
	g_string_append(path, "/modules/erosion/erosion_out_thermal.ppm");
	assert($(bool, image, writeImageToFile)(path->str, surface1));
	g_string_free(path, true);

	path = g_string_new(execpath);
	g_string_append(path, "/modules/erosion/erosion_out_hydraulic.ppm");
	assert($(bool, image, writeImageToFile)(path->str, surface2));
	g_string_free(path, true);

	free(execpath);
#endif
	return true;
}

MODULE_FINALIZE
{
}

static inline void erodeThermalCell(Image* hMap, unsigned int x, unsigned int y, float talusAngle)
{
	const float c = 0.5;
	const float cellSize = 0.01; // NOTE: angle depends on the terrain scale!
	double dMax = 0.0, dTotal = 0.0;
	double T = cellSize * tan(talusAngle);

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,hMap->height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,hMap->width)); j++) {
			if(j == x && i == y)
				continue;

			double di = getImage(hMap, x, y, 0) - getImage(hMap, j, i, 0);
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

	float h = getImage(hMap, x, y, 0);

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,hMap->height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,hMap->width)); j++) {
			if(j == x && i == y)
				continue;

			double di = h - getImage(hMap, j, i, 0);
			if(di > T) {
				double hNew = getImage(hMap, j, i, 0) + c * (dMax-T) * di/dTotal;
				setImage(hMap, j, i, 0, hNew);
			}
		}
	}

	// reduce the cell center to preserve the total amount of material
	float hNew = h - c * (dMax-T);
	setImage(hMap, x, y, 0, hNew);
}

/**
 * Erodes the height map using thermal weathering.
 *
 * @param height_map	height map of the erosion surface
 * @param talusAngle	critical angle of response
 * @param steps			number of iteration steps
 */
API void erodeThermal(Image* hMap, float talusAngle, unsigned int steps)
{
	assert(hMap != NULL);

	if(steps == 0)
		return;

	unsigned int width, height;
	width = hMap->width;
	height = hMap->height;

	// ensure floating point precission
	Image *heightMap = $(Image *, image, copyImage)(hMap, IMAGE_TYPE_FLOAT);

	for(int k=0; k<steps; k++) {
		// erode one time step
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned int x = 0; x < width; x++) {
				erodeThermalCell(heightMap, x, y, talusAngle);
			}
		}
	}

	// copy result to the other height map channels
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			double value = getImage(heightMap, x, y, 0);
			assert(value <= 1.0 && value >= 0.0);
			for(unsigned int c = 0; c < MIN(hMap->channels, 4); c++) {
				setImage(hMap, x, y, c, value);
			}
		}
	}

	// cleanup
	$(void, Image*, freeImage)(heightMap);
}

static inline void waterFlowCell(Image* hMap, Image* waterMap, Image* sedimentMap, unsigned int x, unsigned int y)
{
	unsigned int width = hMap->width;
	unsigned int height = hMap->height;
	unsigned int cellCount = 0;
	double dTotal = 0.0, aTotal = 0.0;
	double w = getImage(waterMap, x, y, 0);
	double h = getImage(hMap, x, y, 0);
	double m = getImage(sedimentMap, x, y, 0);
	double a = h + w;

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,width)); j++) {
			if(j == x && i == y)
				continue;

			double hi = getImage(hMap, j, i, 0);
			double wi = getImage(waterMap, j, i, 0);
			double ai = hi + wi;
			double di = a - ai;
			if(di > 0.0) {
				dTotal += di;
				aTotal += ai;
				cellCount++;
			}
		}
	}

	if(cellCount < 1) {
		return;
	}

	double deltaA = a - (aTotal / (double)cellCount);

	for(unsigned int i=(MAX(y-1,0)); i<(MIN(y+2,height)); i++) {
		for(unsigned int j=(MAX(x-1,0)); j<(MIN(x+2,width)); j++) {
			if(j == x && i == y)
				continue;

			double hi = getImage(hMap, j, i, 0);
			double wi = getImage(waterMap, j, i, 0);
			double mi = getImage(sedimentMap, j, i, 0);
			double ai = hi + wi;
			double di = a - ai;

			if(di > 0.0) {
				// water flow to neighboring cells
				double deltaWi = (MIN(w, deltaA)) * di / dTotal;
				setImage(waterMap, j, i, 0, wi + deltaWi);

				// sediment distribution
				// assumption: all material is uniformly distributed within the water w
				double deltaMi = m * deltaWi / w;
				setImage(sedimentMap, j, i, 0, mi + deltaMi);
				setImage(sedimentMap, x, y, 0, getImage(sedimentMap, x, y, 0) - deltaMi);
			}
		}
	}

	setImage(waterMap, x, y, 0, w - (MIN(w, deltaA)));
}

/**
 * Erodes the height map using hydraulic erosion.
 *
 * @param height_map	height map of the erosion surface
 * @param steps			number of iteration steps
 */
API void erodeHydraulic(Image* hMap, unsigned int steps)
{
	assert(hMap != NULL);

	if(steps == 0)
		return;

	Image *waterMap, *sedimentMap;
	unsigned int width, height;
	const float K_rain = 0.01; // amount of new water per cell
	const float K_solubility = 0.01; // (German: lösbarkeit)
	const float K_evaporation = 0.5;
	const float K_capacity = 0.3;
	const float hScale = 1.0;

	width = hMap->width;
	height = hMap->height;

	// ensure floating point precission
	Image *heightMap = $(Image *, image, copyImage)(hMap, IMAGE_TYPE_FLOAT);
	$(void, image, scaleImageChannel)(heightMap, 0, hScale);

	// create water map
	waterMap = $(Image *, image, createImageFloat)(width, height, 1);
	assert(waterMap != NULL);

	// create map of the sediment in the water
	sedimentMap = $(Image *, image, createImageFloat)(width, height, 1);
	assert(sedimentMap != NULL);

	// init water level and sediment to zero
	memset(waterMap->data.byte_data, 0, width*height*sizeof(float));
	memset(sedimentMap->data.byte_data, 0, width*height*sizeof(float));

	for(int k=0; k<steps; k++) {
		// let it rain
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned int x = 0; x < width; x++) {
				setImage(waterMap, x, y, 0, getImage(waterMap, x, y, 0) + K_rain);
			}
		}

		// erode terrain
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned int x = 0; x < width; x++) {
				//double mMax = getImage(waterMap, x, y, 0) * K_solubility - getImage(sedimentMap, x, y, 0);
				//double deltaM = CLAMP(mMax, 0.f, getImage(heightMap, x, y, 0));
				double deltaM = K_solubility * getImage(waterMap, x, y, 0);
				setImage(heightMap, x, y, 0, getImage(heightMap, x, y, 0) - deltaM);
				setImage(sedimentMap, x, y, 0, getImage(sedimentMap, x, y, 0) + deltaM);
			}
		}

		// water flow and sediment transport
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned int x = 0; x < width; x++) {
				waterFlowCell(heightMap, waterMap, sedimentMap, x, y);
			}
		}

		// evaporate water and accumulate sediments
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned int x = 0; x < width; x++) {
				double h = getImage(heightMap, x, y, 0);
				double m = getImage(sedimentMap, x, y, 0);
				double w = getImage(waterMap, x, y, 0);

				double wNew = w * (1.0 - K_evaporation);
				// NOTE: water will never completely vanish

				double mMax = K_capacity * wNew;
				double deltaM = MAX(0.0, m - mMax);

				setImage(sedimentMap, x, y, 0, m - deltaM);
				setImage(heightMap, x, y, 0, h + deltaM);
				setImage(waterMap, x, y, 0, wNew);
			}
		}
	}

	// evaporate all remaining water
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			double h = getImage(heightMap, x, y, 0);
			double m = getImage(sedimentMap, x, y, 0);
			setImage(heightMap, x, y, 0, h + m);
			// NOTE: unnecessary
			setImage(waterMap, x, y, 0, 0.0);
		}
	}

	// copy values to the original height map channels
	for(unsigned int y = 0; y < height; y++) {
		for(unsigned int x = 0; x < width; x++) {
			double value = getImage(heightMap, x, y, 0)/hScale;
			assert(value <= 1.0 && value >= 0.0);
			for(unsigned int c = 0; c < MIN(hMap->channels, 4); c++) {
				setImage(hMap, x, y, c, value);
			}
		}
	}

	// cleanup
	$(void, Image*, freeImage)(heightMap);
	$(void, Image*, freeImage)(waterMap);
	$(void, Image*, freeImage)(sedimentMap);
}
