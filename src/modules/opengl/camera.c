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
#include "transform.h"
#include "camera.h"

/**
 * Creates a new OpenGL camera positioned at the origin and looking in z direction with positive y axis as up vector
 *
 * @result		the created OpenGL camera
 */
API OpenGLCamera *createOpenGLCamera()
{
	OpenGLCamera *camera = ALLOCATE_OBJECT(OpenGLCamera);
	camera->direction = $(Vector *, linalg, createVector3)(0.0, 0.0, 1.0);
	camera->position = $(Vector *, linalg, createVector3)(0.0, 0.0, 0.0);
	camera->up = $(Vector *, linalg, createVector3)(0.0, 1.0, 0.0);

	return camera;
}

/**
 * Moves an OpenGL camera a certain amount into a certain direction
 *
 * @param camera			the OpenGL camera to move
 * @param move				the move type to move the camera with
 * @param amount			the amount in coordinate units to move the camera into the specified move direction
 */
API void moveOpenGLCamera(OpenGLCamera *camera, OpenGLCameraMove move, double amount)
{
	Vector *step;

	switch(move) {
		case OPENGL_CAMERA_MOVE_FORWARD:
			step = $(Vector *, linalg, copyVector)(camera->direction);
			$(void, linalg, multiplyVectorScalar)(step, amount);
		break;
		case OPENGL_CAMERA_MOVE_BACK:
			step = $(Vector *, linalg, copyVector)(camera->direction);
			$(void, linalg, multiplyVectorScalar)(step, -amount);
		break;
		case OPENGL_CAMERA_MOVE_LEFT:
			step = $(Vector *, linalg, crossVectors)(camera->direction, camera->up);
			$(void, linalg, multiplyVectorScalar)(step, -amount);
		break;
		case OPENGL_CAMERA_MOVE_RIGHT:
			step = $(Vector *, linalg, crossVectors)(camera->direction, camera->up);
			$(void, linalg, multiplyVectorScalar)(step, amount);
		break;
		default:
			LOG_ERROR("Trying to move OpenGL camera into unspecified direction %d", move);
		break;
	}

	$(void, linalg, addVector)(camera->position, step);
	$(void, linalg, freeVector)(step);
}

/**
 * Generates a look-at matrix for an OpenGL camera
 *
 * @param camera		the camera for which to generate a look-at matrix
 * @result				the created look-at matrix
 */
API Matrix *getOpenGLCameraLookAtMatrix(OpenGLCamera *camera)
{
	return createLookIntoDirectionMatrix(camera->position, camera->direction, camera->up);
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
