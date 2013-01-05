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

#ifndef LODMAP_IMAGE_SOURCE_H
#define LODMAP_IMAGE_SOURCE_H

#include "modules/image/image.h"
#include "source.h"


/**
 * Creates an image source for an OpenGL LOD map from a store representation
 *
 * @param store			the store config from which to create the image LOD map source
 * @result				the created data source or NULL on failure
 */
API OpenGLLodMapDataSource *createOpenGLLodMapImageSourceFromStore(Store *store);

/**
 * Creates an image source for an OpenGL LOD map
 *
 * @param heights				the image from which to read the height data (note that the source takes over control over this image, i.e. you must not free it once this function succeeded)
 * @param normals				the image from which to read the normals data or NULL if unused (note that the source takes over control over this image, i.e. you must not free it once this function succeeded)
 * @param texture				the image from which to read the texture data or NULL if unused (note that the source takes over control over this image, i.e. you must not free it once this function succeeded)
 * @param baseLevel				the base level of a tile
 * @param heightRatio			the ratio to scale height values with
 * @result						the created LOD map data source or NULL on failure
 */
API OpenGLLodMapDataSource *createOpenGLLodMapImageSource(Image *heights, Image *normals, Image *texture, unsigned int baseLevel, float heightRatio);

#endif
