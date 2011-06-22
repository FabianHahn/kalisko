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

#include <glib.h>
#include <math.h>
#include <assert.h>
#include "dll.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "random.h"
#include "worley.h"

/**
 * Private Woley noise context struct
 */
struct RandomWorleyContextStruct {
	/** array of all surface centers */
	GPtrArray *points;
	/** number of points to use */
	unsigned int count;
	/** number of spacial dimensions */
	unsigned int dimensions;
};

/**
 * Compare function for float distances
 *
 * @param a		a pointer to the first distance to compare
 * @param b		a pointer to the first distance to compare
 * @result		positive if a > b, zero if a == b, negative if a < b
 */
static int compareDistances(const void *a, const void *b)
{
	if(*(const float*)a < *(const float*)b) {
		return -1;
	} else {
		return *(const float*)a > *(const float*)b;
	}
}

/**
 * Initializes a Worley noise context
 *
 * The caller has to free it with 'freeWorleyContext' after use.
 *
 * @see freeWorleyContext
 * @param count				number of points in space
 * @param dimensions		number of spacial dimensions
 * @return 					a new worley context
 */
API RandomWorleyContext* createWorleyContext(unsigned int count, unsigned int dimensions)
{
	RandomWorleyContext* context = ALLOCATE_OBJECT(RandomWorleyContext);
	context->points = g_ptr_array_new();
	context->count = count;
	context->dimensions = dimensions;

	for(unsigned int i = 0; i < count; i++) {
		Vector *point = $(Vector *, linalg, createVector)(dimensions);
		float *pointData = $(float *, linalg, getVectorData)(point);

		for(unsigned int d = 0; d < dimensions; d++) {
			pointData[d] = randomUniform();
		}

		g_ptr_array_add(context->points, point);
	}

	return context;
}

/**
 * Frees a Worley noise context
 *
 * @param context			the Worley noise context to free
 */
API void freeWorleyContext(RandomWorleyContext *context)
{
	assert(context != NULL);

	for(unsigned int i = 0; i < context->points->len; i++) {
		$(void, linalg, freeVector)(context->points->pdata[i]);
	}
	g_ptr_array_free(context->points, true);
	free(context);
}

/**
 * Computes a sample in a Worley / Voronoi noise pattern
 *
 * Returns the distance to the n'th closest neighbour.
 *
 * @param context		a pointer to a Woley noise context
 * @param query			the query point to lookup
 * @param weights		array of neighbour weights
 * @param weight_count	length of the weight array
 * @param method		distance measurement method
 * @return				Worley noise (> 0.0)
 */
static float randomWorleyArray(RandomWorleyContext *context, Vector *query, int* weights, unsigned int weight_count, RandomWorleyDistance method)
{
	assert(weight_count <= context->count);

	float distances[context->count];

	// compute all distances to the sample point
	for(unsigned int i = 0; i < context->count; i++) {
		Vector *point = context->points->pdata[i];
		Vector *diff = $(Vector *, linalg, diffVectors)(point, query);

		switch(method) {
			case RANDOM_WORLEY_DISTANCE_EUCLIDEAN:
				distances[i] = $(float, linalg, getVectorLength)(diff);
			break;
			case RANDOM_WORLEY_DISTANCE_EUCLIDEAN_SQUARED:
				distances[i] = $(float, linalg, getVectorLength2)(diff);
			break;
			default:
				LOG_ERROR("Tried to compute Worley noise with invalid distance method '%d', aborting", method);
				return 0.0f;
			break;
		}

		$(void, linalg, freeVector)(diff);
	}

	// sort neighbours by the distance
	qsort(distances, context->count, sizeof(float), &compareDistances);

	// sum weighted neighbour distances
	float result = 0;
	for(unsigned int i=0; i<weight_count; i++) {
		result += weights[i]*distances[i];
	}

	return result;
}

/**
 * Computes a sample in a Worley / Voronoi noise pattern
 *
 * Returns the distance to the n'th closest neighbour.
 *
 * @param context		a pointer to a Woley noise context
 * @param query			the query point to lookup
 * @param neighbour		n'th closest neighbour that has influence
 * @param method		distance measurement method
 * @return				Worley noise (> 0.0)
 */
API float randomWorley(RandomWorleyContext *context, Vector *query, unsigned int neighbour, RandomWorleyDistance method)
{
	assert(context != NULL);
	assert(neighbour > 0 && neighbour <= context->count);

	int weights[neighbour];

	for(unsigned int i=0; i<neighbour-1; i++) {
			weights[i] = 0;
	}
	weights[neighbour-1] = 1;

	return randomWorleyArray(context, query, weights, neighbour, method);
}

/**
 * Computes the difference of F2 and F1 Worley / Voronoi noise
 *
 * This function creates crystal shaped areas.
 *
 * @param context		a pointer to a Woley noise context
 * @param query			the query point to lookup
 * @param method		distance measurement method
 * @return				Worley noise (> 0.0)
 */
API float randomWorleyDifference21(RandomWorleyContext *context, Vector *query, RandomWorleyDistance method)
{
	assert(context != NULL);

	int weights[2] = {-1, 1};

	return randomWorleyArray(context, query, weights, 2, method);
}

/**
 * Computes the difference of F3 and F2 Worley / Voronoi noise
 *
 * This function creates crystal shaped areas.
 *
 * @param context		a pointer to a Woley noise context
 * @param query			the query point to lookup
 * @param method		distance measurement method
 * @return				Worley noise (> 0.0)
 */
API float randomWorleyDifference32(RandomWorleyContext *context, Vector *query, RandomWorleyDistance method)
{
	assert(context != NULL);

	int weights[3] = {0, -1, 1};

	return randomWorleyArray(context, query, weights, 3, method);
}
