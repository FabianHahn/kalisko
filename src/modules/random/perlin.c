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

#include <math.h>
#include <assert.h>
#include "dll.h"
#include "api.h"
#include "random.h"
#include "perlin.h"

/**
 * The permutation array used for the perlin noise
 */
static unsigned int *permutation;

/**
 * The gradients used for the perlin noise
 */
static int gradients[] = {
	(4 << 6) + (4 << 3) + 0, // (1,1,0)
	(2 << 6) + (4 << 3) + 0, // (-1,1,0)
	(4 << 6) + (2 << 3) + 0, // (1,-1,0)
	(2 << 6) + (2 << 3) + 0, // (-1,-1,0)
	(4 << 6) + (0 << 3) + 4, // (1,0,1)
	(2 << 6) + (0 << 3) + 4, // (-1,0,1)
	(4 << 6) + (0 << 3) + 2, // (1,0,-1)
	(2 << 6) + (0 << 3) + 2, // (-1,0,-1)
	(0 << 6) + (4 << 3) + 4, // (0,1,1)
	(0 << 6) + (2 << 3) + 4, // (0,-1,1)
	(0 << 6) + (4 << 3) + 2, // (0,1,-1)
	(0 << 6) + (2 << 3) + 2, // (0,-1,-1)
	(4 << 6) + (4 << 3) + 0, // (1,1,0)
	(2 << 6) + (4 << 3) + 0, // (-1,1,0)
	(0 << 6) + (2 << 3) + 4, // (0,-1,1)
	(0 << 6) + (2 << 3) + 2  // (0,-1,-1)
};

/**
 * Initialized the perlin noise generator
 */
API void initPerlin()
{
	permutation = randomPermutation(256);
}

/**
 * Frees the perlin noise generator
 */
API void freePerlin()
{
	free(permutation);
}

static inline float fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline float lerp(float t, float a, float b)
{
	return a + t * (b - a);
}

static inline float gradient(unsigned int corner, float dx, float dy, float dz)
{
	int gradient = gradients[corner & 15]; // extract lower 4 bits = modulo 16, then lookup gradient

	float result = 0.0f;

	// Check x coordinate
	if(gradient & (4 << 6)) { // (1,_,_)
		result += dx;
	} else if(gradient & (2 << 6)) { // (-1,_,_)
		result -= dx;
	}

	// Check y coordinate
	if(gradient & (4 << 3)) { // (_,1,_)
		result += dy;
	} else if(gradient & (2 << 3)) { // (_,-1,_)
		result -= dy;
	}

	// Check z coordinate
	if(gradient & 4) { // (_,_,1)
		result += dz;
	} else if(gradient & 2) { // (_,_,-1)
		result -= dz;
	}

	return result;
}

/**
 * Generates a random perlin noise value from 3D coordinates
 * This follows the description in the paper "Improving Noise" (http://mrl.nyu.edu/~perlin/paper445.pdf) by Ken Perlin
 *
 * @param x		the x coordinate
 * @param y		the y coordinate
 * @param z		the z coordinate
 */
API float randomPerlin(double x, double y, double z)
{
	int X = floor(x);
	int Y = floor(y);
	int Z = floor(z);

	// Compute indices of cube corners
	unsigned int cxl = permutation[X & 255];
	unsigned int cxlyl = permutation[(cxl+Y) & 255];
	unsigned int cxlylzl = permutation[(cxlyl+Z) & 255];
	unsigned int cxlylzh = permutation[(cxlyl+Z+1) & 255];
	unsigned int cxlyh = permutation[(cxl+Y+1) & 255];
	unsigned int cxlyhzl = permutation[(cxlyh+Z) & 255];
	unsigned int cxlyhzh = permutation[(cxlyh+Z+1) & 255];
	unsigned int cxh = permutation[(X+1) & 255];
	unsigned int cxhyl = permutation[(cxh+Y) & 255];
	unsigned int cxhylzl = permutation[(cxhyl+Z) & 255];
	unsigned int cxhylzh = permutation[(cxhyl+Z+1) & 255];
	unsigned int cxhyh = permutation[(cxh+Y+1) & 255];
	unsigned int cxhyhzl = permutation[(cxhyh+Z) & 255];
	unsigned int cxhyhzh = permutation[(cxhyh+Z+1) & 255];

	float dx = x - X;
	float dy = y - Y;
	float dz = z - Z;

	// Compute dot products with corner gradients
	float dxlylzl = gradient(cxlylzl, dx, dy, dz);
	float dxlylzh = gradient(cxlylzh, dx, dy, dz-1);
	float dxlyhzl = gradient(cxlyhzl, dx, dy-1, dz);
	float dxlyhzh = gradient(cxlyhzh, dx, dy-1, dz-1);
	float dxhylzl = gradient(cxhylzl, dx-1, dy, dz);
	float dxhylzh = gradient(cxhylzh, dx-1, dy, dz-1);
	float dxhyhzl = gradient(cxhyhzl, dx-1, dy-1, dz);
	float dxhyhzh = gradient(cxhyhzh, dx-1, dy-1, dz-1);

	float fadex = fade(dx);
	float fadey = fade(dy);
	float fadez = fade(dz);

	// Trilinear interpolation between corners using fade function
	float iylzl = lerp(fadex, dxlylzl, dxhylzl);
	float iyhzl = lerp(fadex, dxlyhzl, dxhyhzl);
	float iylzh = lerp(fadex, dxlylzh, dxhylzh);
	float iyhzh = lerp(fadex, dxlyhzh, dxhyhzh);
	float izl = lerp(fadey, iylzl, iyhzl);
	float izh = lerp(fadey, iylzh, iyhzh);
	return lerp(fadez, izl, izh);
}
