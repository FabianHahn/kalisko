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

#include <assert.h>
#include <glib.h>
#include "dll.h"
#include "api.h"
#include "imagesynth.h"
#include "synthesizers.h"

MODULE_NAME("imagesynth");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to synthesize procedural images");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 2, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("image", 0, 5, 14), MODULE_DEPENDENCY("random", 0, 6, 2), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("linalg", 0, 3, 4));

/**
 * Hash table associating string names with their corresponding image synthesizers
 */
static GHashTable *synthesizers;

MODULE_INIT
{
	synthesizers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	registerImageSynthesizer("fBm", &synthesizeImageFBm);
	registerImageSynthesizer("turbulence", &synthesizeImageFBm);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(synthesizers);
}

/**
 * Registers an image synthesizer
 *
 * @param name				the name of the image synthesizer to register
 * @param synthesizer		the synthesizer that should be registered
 * @result					true if successful
 */
API bool registerImageSynthesizer(const char *name, ImageSynthesizer *synthesizer)
{
	if(g_hash_table_lookup(synthesizers, name) != NULL) {
		LOG_ERROR("Failed to register image synthesizer: A synthesizer with that name is already registered");
		return false;
	}

	g_hash_table_insert(synthesizers, strdup(name), synthesizer);
	return true;
}

/**
 * Unregisters an image synthesizer
 *
 * @param name				the name of the image synthesizer to unregister
 * @result					true if successful
 */
API bool unregisterImageSynthesizer(const char *name)
{
	return g_hash_table_remove(synthesizers, name);
}

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
API Image *synthesizeImage(const char *name, unsigned int width, unsigned int height, unsigned int channels, Store *parameters)
{
	ImageSynthesizer *synthesizer;
	if((synthesizer = g_hash_table_lookup(synthesizers, name)) == NULL) {
		LOG_ERROR("Failed to synthesize image: No such synthesizer with name '%s' found", name);
		return NULL;
	}

	return synthesizer(name, width, height, channels, parameters);
}
