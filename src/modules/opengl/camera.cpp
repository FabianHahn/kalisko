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

extern "C" {
#include <glib.h>
#include <GL/glew.h>
}

#include "dll.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/transform.h"
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
	Vector step(3);

	switch(move) {
		case OPENGL_CAMERA_MOVE_FORWARD:
			step = amount * *camera->direction;
		break;
		case OPENGL_CAMERA_MOVE_BACK:
			step = -amount * *camera->direction;
		break;
		case OPENGL_CAMERA_MOVE_LEFT:
			step = -amount * (*camera->direction % *camera->up);
		break;
		case OPENGL_CAMERA_MOVE_RIGHT:
			step = amount * (*camera->direction % *camera->up);
		break;
		case OPENGL_CAMERA_MOVE_UP:
			step = amount * *camera->up;
		break;
		case OPENGL_CAMERA_MOVE_DOWN:
			step = -amount * *camera->up;
		break;
		default:
			LOG_ERROR("Trying to move OpenGL camera into unspecified direction %d", move);
		break;
	}

	(*camera->position) += step;
}

/**
 * Tilts an OpenGL camera a certain amount into a certain direction
 *
 * @param camera			the OpenGL camera to tilt
 * @param tilt				the tilt type to tilt the camera with
 * @param angle				the angle in radians to tilt the camera into the specified move direction
 */
API void tiltOpenGLCamera(OpenGLCamera *camera, OpenGLCameraTilt tilt, double angle)
{
	Vector *rightDirection = $(Vector *, linalg, crossVectors)(camera->direction, camera->up);
	Vector *axis;

	switch(tilt) {
		case OPENGL_CAMERA_TILT_UP:
			axis = $(Vector *, linalg, copyVector)(rightDirection);
		break;
		case OPENGL_CAMERA_TILT_DOWN:
			axis = $(Vector *, linalg, copyVector)(rightDirection);
			$(void, linalg, multiplyVectorScalar)(axis, -1);
		break;
		case OPENGL_CAMERA_TILT_LEFT:
			axis = $(Vector *, linalg, copyVector)(camera->up);
		break;
		case OPENGL_CAMERA_TILT_RIGHT:
			axis = $(Vector *, linalg, copyVector)(camera->up);
			$(void, linalg, multiplyVectorScalar)(axis, -1);
		break;
		default:
			LOG_ERROR("Trying to tilt OpenGL camera into unspecified direction %d", tilt);
		break;
	}

	// Rotate camera direction
	Matrix *rotation = $(Matrix *, linalg, createRotationMatrix)(axis, angle);
	Matrix *normalRotation = $(Matrix *, linalg, transposeMatrix)(rotation);
	Vector *newDirection = $(Vector *, linalg, multiplyMatrixVector)(normalRotation, camera->direction);
	$(void, linalg, homogenizeVector)(newDirection);
	Vector *newRightDirection = $(Vector *, linalg, multiplyMatrixVector)(normalRotation, rightDirection);
	$(void, linalg, homogenizeVector)(newRightDirection);

	// Enforce no camera roll
	$(void, linalg, setVector)(newRightDirection, 1, 0.0);
	Vector *newUp = $(Vector *, linalg, crossVectors)(newRightDirection, newDirection);

	// Set new camera vectors
	$(void, linalg, assignVector)(camera->direction, newDirection);
	$(void, linalg, normalizeVector)(camera->direction);
	$(void, linalg, assignVector)(camera->up, newUp);
	$(void, linalg, normalizeVector)(camera->up);

	// Check if we're still correctly turned
	Vector *check = $(Vector *, linalg, crossVectors)(camera->direction, camera->up);
	if($(float, linalg, dotVectors)(check, newRightDirection) < 0.0f) {
		// Correct orientation
		$(void, linalg, multiplyVectorScalar)(camera->up, -1);
	}

	// Clean up
	$(void, linalg, freeMatrix)(rotation);
	$(void, linalg, freeMatrix)(normalRotation);
	$(void, linalg, freeVector)(rightDirection);
	$(void, linalg, freeVector)(axis);
	$(void, linalg, freeVector)(newDirection);
	$(void, linalg, freeVector)(newRightDirection);
	$(void, linalg, freeVector)(newUp);
}

/**
 * Generates a look-at matrix for an OpenGL camera
 *
 * @param camera		the camera for which to generate a look-at matrix
 * @result				the created look-at matrix
 */
API Matrix *getOpenGLCameraLookAtMatrix(OpenGLCamera *camera)
{
	return $(Matrix *, linalg, createLookIntoDirectionMatrix)(camera->position, camera->direction, camera->up);
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
