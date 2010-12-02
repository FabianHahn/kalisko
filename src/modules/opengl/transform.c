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

#include <math.h>
#include <assert.h>
#include <glib.h>
#include <GL/glew.h>
#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "api.h"
#include "transform.h"

/**
 * Creates a look-at matrix which transforms the world into a coordinate system as seen from a camera at position eye looking at focus with specified up vector
 *
 * @param eye		the position of the camera ("the eye")
 * @param focus		the focused point at which the camera is looking
 * @param up		the up vector for the camera coordinate system - as a side-effect, this vector is normalized
 * @result			the created loot-at matrix
 */
API Matrix *createLookAtMatrix(Vector *eye, Vector *focus, Vector *up)
{
	assert($(unsigned int, linalg, getVectorSize)(eye) == 3);
	assert($(unsigned int, linalg, getVectorSize)(focus) == 3);
	assert($(unsigned int, linalg, getVectorSize)(up) == 3);

	Vector *f = $(Vector *, linalg, diffVectors)(focus, eye);
	Matrix *lookAt = createLookIntoDirectionMatrix(eye, f, up);
	$(void, linalg, freeVector)(f);

	return lookAt;
}

/**
 * Creates a look-at matrix which transforms the world into a coordinate system as seen from a camera at position eye looking into a direction with specified up vector
 *
 * @param eye		the position of the camera ("the eye")
 * @param f			the direction into which the camera is looking - as a side-effect, this vector is normalized
 * @param up		the up vector for the camera coordinate system - as a side-effect, this vector is normalized
 * @result			the created loot-at matrix
 */
API Matrix *createLookIntoDirectionMatrix(Vector *eye, Vector *f, Vector *up)
{
	assert($(unsigned int, linalg, getVectorSize)(eye) == 3);
	assert($(unsigned int, linalg, getVectorSize)(f) == 3);
	assert($(unsigned int, linalg, getVectorSize)(up) == 3);

	// Construct camera coordinate system basis
	$(void, linalg, normalizeVector)(f);
	$(void, linalg, normalizeVector)(up);

	Vector *s = $(Vector *, linalg, crossVectors)(f, up);
	Vector *u = $(Vector *, linalg, crossVectors)(s, f);

	float *eyeData = $(float *, linalg, getVectorData)(eye);
	float *sData = $(float *, linalg, getVectorData)(s);
	float *uData = $(float *, linalg, getVectorData)(u);
	float *fData = $(float *, linalg, getVectorData)(f);

	// Construct shift matrix to camera position
	Matrix *shift = $(Matrix *, linalg, createMatrix)(4, 4);
	$(void, linalg, eyeMatrix)(shift);
	float *shiftData = $(float *, linalg, getMatrixData)(shift);

	shiftData[0*4+3] = -eyeData[0];
	shiftData[1*4+3] = -eyeData[1];
	shiftData[2*4+3] = -eyeData[2];

	// Construct basis transform to camera coordinates
	Matrix *transform = $(Matrix *, linalg, createMatrix)(4, 4);
	$(void, linalg, eyeMatrix)(transform);
	float *transformData = $(float *, linalg, getMatrixData)(transform);

	transformData[0*4+0] = sData[0];
	transformData[0*4+1] = sData[1];
	transformData[0*4+2] = sData[2];
	transformData[1*4+0] = uData[0];
	transformData[1*4+1] = uData[1];
	transformData[1*4+2] = uData[2];
	transformData[2*4+0] = -fData[0];
	transformData[2*4+1] = -fData[1];
	transformData[2*4+2] = -fData[2];

	// Create look at matrix
	Matrix *lookAt = $(Matrix *, linalg, multiplyMatrices)(transform, shift);

	// Clean up
	$(void, linalg, freeVector)(s);
	$(void, linalg, freeVector)(u);
	$(void, linalg, freeMatrix)(shift);
	$(void, linalg, freeMatrix)(transform);

	return lookAt;
}

/**
 * Creates a perspective matrix that projects point from the world coordinate system into the camera coordinate system (with depth values as z)
 *
 * @param fovy		the viewing angle of the camera in radians
 * @param ar		the aspect ratio of the camera (width / height)
 * @param znear		the projection plane of the camera
 * @param zfar		the back plane of the camera viewing volume
 */
API Matrix *createPerspectiveMatrix(double fovy, double ar, double znear, double zfar)
{
	Matrix *perspective = $(Matrix *, linalg, createMatrix)(4, 4);
	$(void, linalg, clearMatrix)(perspective);
	float *perspectiveData = $(float *, linalg, getMatrixData)(perspective);

	float f = 1.0f / tan(fovy / 2.0f);

	perspectiveData[0*4+0] = f / ar;
	perspectiveData[1*4+1] = f;
	perspectiveData[2*4+2] = (zfar + znear) / (znear - zfar);
	perspectiveData[2*4+3] = (2.0f * zfar * znear) / (znear - zfar);
	perspectiveData[3*4+2] = -1.0f;

	return perspective;
}
