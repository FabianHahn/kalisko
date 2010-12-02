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
#include <GL/freeglut.h>
#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "api.h"
#include "camera.h"

/**
 * Creates a new OpenGL camera positioned at the origin and looking in z direction with positive y axis as up vector
 *
 * @result		the created OpenGL camera
 */
API OpenGLCamera *createOpenGLCamera()
{
	OpenGLCamera *camera = ALLOCATE_OBJECT(OpenGLCamera);
	camera->direction = $(Vector *, linalg, createVector3)(0, 0, 1);
	camera->position = $(Vector *, linalg, createVector3)(0, 0, 0);
	camera->up = $(Vector *, linalg, createVector3)(0, 1, 0);

	return camera;
}

/**
 * Frees an OpenGL camera
 *
 * @param camera		the camera to free
 */
API void freeOpenGLCamera(OpenGLCamera *camera)
{
	$(void, linalg, freeVector)(camera->direction);
	$(void, linalg, freeVector)(camera->position);
	$(void, linalg, freeVector)(camera->up);
	free(camera);
}
