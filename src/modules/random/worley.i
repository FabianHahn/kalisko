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

#ifndef RANDOM_WORLEY_H
#define RANDOM_WORLEY_H

#include "modules/linalg/Vector.h"

/**
 * Enum listing possible distance functions for worley noise
 */
typedef enum {
	/** Standard Euclidean (L2 norm) distance function */
	RANDOM_WORLEY_DISTANCE_EUCLIDEAN,
	/** Standard squared Euclidean distance function */
	RANDOM_WORLEY_DISTANCE_EUCLIDEAN_SQUARED
} RandomWorleyDistance;

/** Forward declaration of private struct for Worley noise context */
struct RandomWorleyContextStruct;
typedef struct RandomWorleyContextStruct RandomWorleyContext;

API RandomWorleyContext* createWorleyContext(unsigned int count, unsigned int dimensions);
API void freeWorleyContext(RandomWorleyContext *context);
API float randomWorley(RandomWorleyContext *context, Vector *query, unsigned int neighbour, RandomWorleyDistance method);
API float randomWorleyDifference21(RandomWorleyContext *context, Vector *query, RandomWorleyDistance method);
API float randomWorleyDifference32(RandomWorleyContext *context, Vector *query, RandomWorleyDistance method);

#endif