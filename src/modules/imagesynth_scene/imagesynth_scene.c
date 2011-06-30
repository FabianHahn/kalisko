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
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/opengl/texture.h"
#include "modules/image/image.h"
#include "modules/imagesynth/imagesynth.h"
#include "modules/scene/scene.h"
#include "modules/scene/texture.h"
#include "api.h"
#include "imagesynth_scene.h"

MODULE_NAME("imagesynth_scene");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Scene plugin to support adding textures generated by imagesynth");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("imagesynth", 0, 2, 2), MODULE_DEPENDENCY("scene", 0, 7, 2), MODULE_DEPENDENCY("image", 0, 5, 14), MODULE_DEPENDENCY("store", 0, 6, 11));

MODULE_INIT
{
	return $(bool, scene, registerOpenGLTextureSceneParser)("imagesynth", &parseOpenGLSceneTextureImagesynth);
}

MODULE_FINALIZE
{
	$(bool, scene, unregisterOpenGLTextureSceneParser)("imagesynth");
}

/**
 * OpenGLTextureScene parser for procedurally generated imagesynth textures
 *
 * @param scene			the scene to parse the OpenGL texture for
 * @param path_prefix	the path prefix that should be prepended to any file loaded while parsing
 * @param name			the name of the primitive to parse
 * @param store			the store representation of the OpenGLTexture to parse
 * @result				the parsed OpenGLTexture or NULL on failure
 */
API OpenGLTexture *parseOpenGLSceneTextureImagesynth(Scene *scene, const char *path_prefix, const char *name, Store *store)
{
	Store *synthesizerParam;
	if((synthesizerParam = $(Store *, store, getStorePath)(store, "synthesizer")) == NULL || synthesizerParam->type != STORE_STRING) {
		LOG_ERROR("Failed to parse OpenGL scene texture '%s' from imagesynth source - string parameter 'synthesizer' not found", name);
		return NULL;
	}

	Store *widthParam;
	if((widthParam = $(Store *, store, getStorePath)(store, "width")) == NULL || widthParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene texture '%s' from imagesynth source - integer parameter 'width' not found", name);
		return NULL;
	}

	Store *heightParam;
	if((heightParam = $(Store *, store, getStorePath)(store, "height")) == NULL || heightParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene texture '%s' from imagesynth source - integer parameter 'height' not found", name);
		return NULL;
	}

	Store *channelsParam;
	if((channelsParam = $(Store *, store, getStorePath)(store, "channels")) == NULL || channelsParam->type != STORE_INTEGER) {
		LOG_ERROR("Failed to parse OpenGL scene texture '%s' from imagesynth source - integer parameter 'channels' not found", name);
		return NULL;
	}

	Store *parametersParam;
	if((parametersParam = $(Store *, store, getStorePath)(store, "parameters")) == NULL || parametersParam->type != STORE_ARRAY) {
		LOG_ERROR("Failed to parse OpenGL scene texture '%s' from imagesynth source - array parameter 'parameters' not found", name);
		return NULL;
	}

	Image *image = $(Image *, imagesynth, synthesizeImage)(synthesizerParam->content.string, widthParam->content.integer, heightParam->content.integer, channelsParam->content.integer, parametersParam);

	if(image == NULL) {
		LOG_ERROR("Failed to generate image file from imagesynth source for texture '%s'", name);
		return NULL;
	}

	OpenGLTexture *texture;
	if((texture = $(OpenGLTexture *, opengl, createOpenGLTexture2D)(image, true)) == NULL) {
		LOG_ERROR("Failed to create OpenGL texture '%s' for scene", name);
		$(void, image, freeImage)(image);
		return NULL;
	}

	return texture;
}