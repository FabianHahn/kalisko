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
#define API
#include "source.h"

/**
 * Queries an OpenGL LOD map data source
 *
 * @param dataSource			the data source to query
 * @param query					the type of query to perform
 * @param x						the x position of the tile to query
 * @param y						the y position of the tile to query
 * @param level					the LOD level at which to perform the query
 * @result						the result of the query or NULL if the source doesn't provide data for this kind of query
 */
API Image *queryOpenGLLodMapDataSource(OpenGLLodMapDataSource *dataSource, OpenGLLodMapDataSourceQueryType query, int x, int y, unsigned int level)
{
	OpenGLLodMapDataSourceProviderType provides;

	switch(query) {
		case OPENGL_LODMAP_DATASOURCE_QUERY_HEIGHT:
			provides = dataSource->providesHeight;
		break;
		case OPENGL_LODMAP_DATASOURCE_QUERY_NORMALS:
			provides = dataSource->providesNormals;
		break;
		case OPENGL_LODMAP_DATASOURCE_QUERY_TEXTURE:
			provides = dataSource->providesTexture;
		break;
		default:
			provides = OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE;
		break;
	}

	if(provides == OPENGL_LODMAP_DATASOURCE_PROVIDE_NONE || (provides == OPENGL_LODMAP_DATASOURCE_PROVIDE_LEAF && level > 0)) { // return NULL if we don't provide this kind of query
		return NULL;
	}

	return dataSource->load(dataSource, query, x, y, level);
}
