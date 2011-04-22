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
#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "api.h"
#include "random.h"

MODULE_NAME("random");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Randomness functions");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Returns a random gaussian number with specified distribution
 *
 * @param mean		the mean value of the gaussian distribution
 * @param std		the standard deviation of the gaussian distribution
 * @result			the gaussian random number
 */
API float randomGaussian(double mean, double std)
{
	float x1, x2, w;

	do {
		x1 = 2.0f * randomUniform() - 1.0f;
		x2 = 2.0f * randomUniform() - 1.0f;
		w = x1 * x1 + x2 * x2;
	} while(w >= 1.0f);

	w = sqrt((-2.0f * log(w)) / w);
	return mean + std * x1 * w;
}