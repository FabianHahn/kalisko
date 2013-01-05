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

#ifndef LANDSCAPE_LANDSCAPE_H
#define LANDSCAPE_LANDSCAPE_H

#include "modules/image/image.h"


/**
 * Generates a random landscape heightmap from procedural noise
 *
 * A reasonable value for each parameter is provided in brackets.
 *
 * @param width							the width of the heightmap to generate
 * @param height						the height of the heightmap to generate
 * @param worleyPoints					number of worley points (16)
 * @param fbmFrequency					the frequency of the franctional Brownian noise (4)
 * @param fbmPersistance				the persistence of the franctional Brownian noise (0.5)
 * @param fbmDepth						the number of octaves to overlay for the fractional Brownian noise (6)
 * @param erosionThermalIterations		number of thermal iterations (50)
 * @param erosionThermalTalusAngle		the angle of response in degrees (40)
 * @param erosionHydraulicIterations	number of hydraulic iterations (80)
 * @return								the created landscape heightmap or NULL on error
 */
API Image *generateLandscapeHeightmap(unsigned int width, unsigned int height, unsigned int worleyPoints, double fbmFrequency, double fbmPersistance, unsigned int fbmDepth, unsigned int erosionThermalIterations, double erosionThermalTalusAngle, unsigned int erosionHydraulicIterations);

#endif
