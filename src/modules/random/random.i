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

#ifndef RANDOM_RANDOM_H
#define RANDOM_RANDOM_H

#include <stdlib.h>

/**
 * Returns a random float number between 0 and 1
 *
 * @result			the generated random number
 */
static inline float randomUniform()
{
	return (float) rand() / RAND_MAX;
}

/**
 * Returns a random integer number between specified min and max values
 *
 * @param min		the minimum number to generate
 * @param max		the maximum number to generate
 * @result			the generated random number between min and max (inclusive)
 */
static inline int randomUniformInteger(int min, int max)
{
	return min + (max - min) * randomUniform();
}


/**
 * Returns a random gaussian number with specified distribution
 *
 * @param mean		the mean value of the gaussian distribution
 * @param std		the standard deviation of the gaussian distribution
 * @result			the gaussian random number
 */
API float randomGaussian(double mean, double std);
API unsigned int *randomPermutation(unsigned int size);

#endif
