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
#include <GL/glew.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/event/event.h"
#include "modules/image/image.h"

#define API
#include "opengl.h"
#include "material.h"
#include "model.h"
#include "shader.h"


MODULE_NAME("opengl");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengl module supports hardware accelerated graphics rendering and interaction");
MODULE_VERSION(0, 29, 12);
MODULE_BCVERSION(0, 29, 6);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("linalg", 0, 2, 3), MODULE_DEPENDENCY("image", 0, 5, 20));

MODULE_INIT
{
	initOpenGLMaterials();
	initOpenGLUniforms();

	return true;
}

MODULE_FINALIZE
{
	freeOpenGLMaterials();
	freeOpenGLUniforms();
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

/**
 * Returns a screenshot of the currently active OpenGL framebuffer
 *
 * @param x			the x corner coordinate of the screenshot to take
 * @param y			the y corner coordinate of the screenshot to take
 * @param width		the width of the screenshot to take
 * @param height	the height of the screenshot to take
 * @result			the screenshot as image
 */
API Image *getOpenGLScreenshot(int x, int y, unsigned int width, unsigned int height)
{
	Image *image = $(Image *, image, createImageByte)(width, height, 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); // don't align the returned image
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, image->data.byte_data);

	// OpenGL orients the y axis differently, so flip the image
	Image *flipped = $(Image *, image, flipImage)(image, IMAGE_FLIP_Y);
	$(void, image, freeImage)(image);

	return flipped;
}
