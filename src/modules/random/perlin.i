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

#ifndef RANDOM_PERLIN_H
#define RANDOM_PERLIN_H


/**
 * Initialized the perlin noise generator
 */
API void initPerlin();

/**
 * Frees the perlin noise generator
 */
API void freePerlin();

/**
 * Generates a random perlin noise value from 3D coordinates
 * This follows the description in the paper "Improving Noise" (http://mrl.nyu.edu/~perlin/paper445.pdf) by Ken Perlin
 *
 * @param x		the x coordinate
 * @param y		the y coordinate
 * @param z		the z coordinate
 * @return		returns a random perlin noise value within the bounds of [-sqrt(2), sqrt(2)]
 */
API float randomPerlin(double x, double y, double z);

/**
 * Generates fractional Brownian motion (fBm) noise
 * Reference: http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
 *
 * @param x					the x coordinate
 * @param y					the y coordinate
 * @param z					the z coordinate
 * @param persistence		the persistence of the franctional Brownian noise to generate, should lie in (0,1) and specifies how much further octave levels contribute to the noise value
 * @param depth				the number of octaves to overlay for the fractional Brownian noise
 * @return					a random fractional Brownian motion noise value
 */
API float noiseFBm(double x, double y, double z, double persistence, unsigned int depth);

/**
 * Generates turbulence noise
 *
 * @param x					the x coordinate
 * @param y					the y coordinate
 * @param z					the z coordinate
 * @param persistence		the persistence of the turbulence noise to generate, should lie in (0,1) and specifies how much further octave levels contribute to the noise value
 * @param depth				the number of octaves to overlay for the fractional Brownian noise
 * @return					a random turbulence noise value
 */
API float noiseTurbulence(double x, double y, double z, double persistence, unsigned int depth);

#endif
