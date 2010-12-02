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

#ifndef OPENGL_CAMERA_H
#define OPENGL_CAMERA_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"

/**
 * Describes an OpenGL camera move
 */
typedef enum {
	/** Move the camera forward */
	CAMERA_MOVE_FORWARD,
	/** Move the camera backwards */
	CAMERA_MOVE_BACK,
	/** Move the camera to the left */
	CAMERA_MOVE_LEFT,
	/** Move the camera to the right */
	CAMERA_MOVE_RIGHT
} OpenGLCameraMove;

/**
 * Describes an OpenGL camera tilt
 */
typedef enum {
	/** Tilt the camera upwards */
	CAMERA_TILT_UP,
	/** Tilt the camera downwards */
	CAMERA_TILT_DOWN,
	/** Tilt the camera to the left */
	CAMERA_TILT_LEFT,
	/** Tilt the camera to the right */
	CAMERA_TILT_RIGHT
} OpenGLCameraTilt;

typedef struct {
	/** The current up direction of the camera */
	Vector *up;
	/** The looking direction of the camera */
	Vector *direction;
	/** The position of the camera */
	Vector *position;
} OpenGLCamera;

API OpenGLCamera *createOpenGLCamera();
API void freeOpenGLCamera(OpenGLCamera *camera);

#endif
