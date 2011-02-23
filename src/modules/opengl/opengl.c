/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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
#include <GL/glew.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/event/event.h"

#include "api.h"
#include "opengl.h"
#include "material.h"
#include "model.h"


MODULE_NAME("opengl");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengl module supports hardware accelerated graphics rendering and interaction");
MODULE_VERSION(0, 12, 0);
MODULE_BCVERSION(0, 11, 3);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("linalg", 0, 2, 3), MODULE_DEPENDENCY("mesh", 0, 4, 5));

MODULE_INIT
{
	initOpenGLMaterials();
	initOpenGLModels();

	return true;
}

MODULE_FINALIZE
{
	freeOpenGLMaterials();
	freeOpenGLModels();
}

/**
 * Checks whether an OpenGL error has occurred
 *
 * @result		true if an error occurred
 */
API bool checkOpenGLError()
{
	GLenum err = glGetError();

	if(err != GL_NO_ERROR) {
		const GLubyte *errstr = gluErrorString(err);
		if(errstr != NULL) {
			LOG_ERROR("OpenGL error #%d: %s", err, errstr);
		}

		return true;
	}

	return false;
}
