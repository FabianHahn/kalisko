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
#include "worley.h"

/**
 * A 3d point
 */
typedef struct {
	float x;
	float y;
	float z;
} Point;

/**
 * Private Woley noise context struct
 */
struct RandomWorleyCtxImpl {
	Point *points; //!< array of all surface centers
	unsigned int point_count; //!< number of surface centers
	unsigned int dimensions; //!< number of spacial dimensions
};

/**
 * Initializes a Worley noise context
 *
 * The caller has to free it with 'freeWorleyCtx' after use.
 *
 * @param count			number of points in space
 * @param dimensions	number of spacial dimensions
 * @return 				a new worley context
 */
API RandomWorleyCtx* createWorleyCtx(unsigned int count, unsigned int dimensions)
{
	RandomWorleyCtx* ctx = g_malloc(sizeof(RandomWorleyCtx));
	ctx->points = NULL;
	ctx->point_count = 0;

	updateWorleyCtx(ctx, count, dimensions);

	return ctx;
}

/**
 * Frees a Worley noise context
 */
API void freeWorleyCtx(RandomWorleyCtx* ctx)
{
	assert(ctx != NULL);

	if(ctx->points != NULL) {
		g_free(ctx->points);
	}

	g_free(ctx);
}

/**
 * Updates a Worley noise context
 *
 * @param ctx			a pointer to a Worley noise contex
 * @param count			number of points in space
 * @param dimensions	number of spacial dimensions
 */
API void updateWorleyCtx(RandomWorleyCtx* ctx, unsigned int count, unsigned int dimensions)
{
	if(ctx->points != NULL) {
		g_free(ctx->points);
	}

	ctx->points = g_malloc(count * sizeof(Point));
	ctx->point_count = count;
	ctx->dimensions = dimensions;

	for(int i=0; i<ctx->point_count; i++) {
		if(dimensions == 1) {
			ctx->points[i].x = randomUniform();
			ctx->points[i].y = 0.f;
			ctx->points[i].z = 0.f;
		} else if(dimensions == 2) {
			ctx->points[i].x = randomUniform();
			ctx->points[i].y = randomUniform();
			ctx->points[i].z = 0.f;
		} else if(dimensions > 2) {
			ctx->points[i].x = randomUniform();
			ctx->points[i].y = randomUniform();
			ctx->points[i].z = randomUniform();
		}
	}
}

static int compare(const void *a, const void *b)
{
	if(*(const float*)a < *(const float*)b)
		return -1;
	return *(const float*)a > *(const float*)b;
}

/**
 * Computes a sample in a Worley / Voronoi noise pattern
 *
 * @param ctx			a pointer to a Woley noise context
 * @param x				the x coordinate
 * @param y				the y coordinate
 * @param z				the z coordinate
 * @param method		distance measurement method
 * @param neighbours	number of points that have influence
 * @return				Worley noise (> 0.0)
 */
API float randomWorley(RandomWorleyCtx* ctx, double x, double y, double z, unsigned int neighbours, RandomDistanceMethod method)
{
	float d[ctx->point_count];
	float result = 0.f;

	// compute all distances to the sample point
	for(int i=0; i<ctx->point_count; i++) {
		Point p = ctx->points[i];
		float tx = p.x - x;
		float ty = p.y - y;
		float tz = p.z - z;

		switch(method) {
			case RANDOM_DIST_EUCLIDEAN:
				d[i] = sqrt(tx*tx + ty*ty + tz*tz);
				break;
			case RANDOM_DIST_EUCLIDEAN_SQUARED:
				d[i] = tx*tx + ty*ty + tz*tz;
				break;
			default:
				assert(0); // invalid method
		}
	}

	// sum up the 'neighbours' closest distances
	qsort(d, ctx->point_count, sizeof(float), &compare);

	for(int i=0; i<neighbours; i++) {
		result += d[i];
	}

	return result;
}
