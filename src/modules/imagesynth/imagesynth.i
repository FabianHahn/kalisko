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

#ifndef IMAGESYNTH_IMAGESYNTH_H
#define IMAGESYNTH_IMAGESYNTH_H

#include "modules/image/image.h"
#include "modules/store/store.h"

typedef Image *(ImageSynthesizer)(const char *name, unsigned int width, unsigned int height, unsigned int channels, Store *parameters);


/**
 * Registers an image synthesizer
 *
 * @param name				the name of the image synthesizer to register
 * @param synthesizer		the synthesizer that should be registered
 * @result					true if successful
 */
API bool registerImageSynthesizer(const char *name, ImageSynthesizer *synthesizer);

/**
 * Unregisters an image synthesizer
 *
 * @param name				the name of the image synthesizer to unregister
 * @result					true if successful
 */
API bool unregisterImageSynthesizer(const char *name);

/**
 * Synthesizes an image
 *
 * @param name			the name of the synthesizer to use to produce the image
 * @param width			the width of the image to synthesize
 * @param height		the height of the image to synthesize
 * @param channels		the number of channels for the image to synthesize
 * @param parameters	store representation of custom parameters to be passed to the synthesizer
 * @result				the synthesized image or NULL on failure
 */
API Image *synthesizeImage(const char *name, unsigned int width, unsigned int height, unsigned int channels, Store *parameters);

#endif
