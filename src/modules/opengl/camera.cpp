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
#include "shader.h"

/**
 * Creates a new OpenGL camera positioned at the origin and looking in z direction with positive y axis as up vector
 *
 * @result		the created OpenGL camera
 */
API OpenGLCamera *createOpenGLCamera()
{
	OpenGLCamera *camera = ALLOCATE_OBJECT(OpenGLCamera);
	camera->lookAt = $(Matrix *, linalg, createMatrix)(4, 4);
	camera->direction = $(Vector *, linalg, createVector3)(0.0, 0.0, 1.0);
	camera->position = $(Vector *, linalg, createVector3)(0.0, 0.0, 0.0);
	camera->up = $(Vector *, linalg, createVector3)(0.0, 1.0, 0.0);
	updateOpenGLCameraLookAtMatrix(camera);

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

	*camera->position += step;
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
	Vector rightDirection = *camera->direction % *camera->up;
	Vector axis(3);

	switch(tilt) {
		case OPENGL_CAMERA_TILT_UP:
			axis = rightDirection;
		break;
		case OPENGL_CAMERA_TILT_DOWN:
			axis = -rightDirection;
		break;
		case OPENGL_CAMERA_TILT_LEFT:
			axis = *camera->up;
		break;
		case OPENGL_CAMERA_TILT_RIGHT:
			axis = -*camera->up;
		break;
		default:
			LOG_ERROR("Trying to tilt OpenGL camera into unspecified direction %d", tilt);
		break;
	}

	// Rotate camera direction
	Matrix *rotation = $(Matrix *, linalg, createRotationMatrix)(&axis, angle);
	Matrix normalRotation = rotation->transpose();
	Vector newDirection = (normalRotation * *camera->direction).homogenize();
	Vector newRightDirection = (normalRotation * rightDirection).homogenize();

	// Enforce no camera roll
	newRightDirection[1] = 0.0;
	Vector newUp = newRightDirection % newDirection;

	// Enforce now upside down
	if(newUp[1] >= 0.0f) {
		// Set new camera vectors
		*camera->direction = newDirection.normalize();
		*camera->up = newUp.normalize();
	}

	// Clean up
	$(void, linalg, freeMatrix)(rotation);
}

/**
 * Updates the look-at matrix for an OpenGL camera
 *
 * @param camera		the camera for which to update the look-at matrix
 */
API void updateOpenGLCameraLookAtMatrix(OpenGLCamera *camera)
{
	$(void, linalg, updateLookIntoDirectionMatrix)(camera->lookAt, camera->position, camera->direction, camera->up);
}

/**
 * Activates an OpenGL camera by setting its look-at matrix as global "camera" shader uniform
 *
 * @param camera		the OpenGL camera to activate
 */
API void activateOpenGLCamera(OpenGLCamera *camera)
{
	delOpenGLGlobalShaderUniform("camera");
	OpenGLUniform *uniform = createOpenGLUniformMatrix(camera->lookAt);
	addOpenGLGlobalShaderUniform("camera", uniform);
}

/**
 * Frees an OpenGL camera
 *
 * @param camera		the camera to free
 */
API void freeOpenGLCamera(OpenGLCamera *camera)
{
	$(void, linalg, freeMatrix)(camera->lookAt);
	$(void, linalg, freeVector)(camera->direction);
	$(void, linalg, freeVector)(camera->position);
	$(void, linalg, freeVector)(camera->up);
	free(camera);
}
