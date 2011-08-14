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
#include "dll.h"
#include "modules/image/image.h"
#include "api.h"
#include "source.h"
#include "sources.h"

/**
 * Creates a null source for an OpenGL LOD map, i.e. a source that doesn't provide any data at all
 *
 * @param baseLevel				the base level of a tile for the null source
 * @result						the created LOD map data source
 */
API OpenGLLodMapDataSource *createOpenGLLodMapNullSource(unsigned int baseLevel)
{
	OpenGLLodMapDataSource *source = ALLOCATE_OBJECT(OpenGLLodMapDataSource);
	source->baseLevel = baseLevel;
	source->providesHeight = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->providesNormals = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->providesTexture = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
	source->load = NULL; // if we don't provide anything, we will never be called

	return source;
}
