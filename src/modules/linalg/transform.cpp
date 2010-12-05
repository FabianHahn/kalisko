/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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

#include <cmath>
#include <cassert>
#include "dll.h"

extern "C" {
#include "log.h"
#include "memory_alloc.h"
#include "api.h"
}

#include "transform.h"
#include "Vector.h"
#include "Matrix.h"

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
	assert(eye->getSize() == 3);
	assert(focus->getSize() == 3);
	assert(up->getSize() == 3);

	Vector f = *focus - *eye;
	return createLookIntoDirectionMatrix(eye, &f, up);
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
	assert(eye->getSize() == 3);
	assert(f->getSize() == 3);
	assert(up->getSize() == 3);

	// Construct camera coordinate system basis
	f->normalize();
	up->normalize();

	Vector s = *f % *up;
	Vector u = s % *f;

	// Construct shift matrix to camera position
	Matrix shift(4, 4);
	shift.identity();

	shift(0, 3) = -(*eye)[0];
	shift(1, 3) = -(*eye)[1];
	shift(2, 3) = -(*eye)[2];

	// Construct basis transform to camera coordinates
	Matrix transform(4, 4);
	transform.identity();

	transform(0, 0) = s[0];
	transform(0, 1) = s[1];
	transform(0, 2) = s[2];
	transform(1, 0) = u[0];
	transform(1, 1) = u[1];
	transform(1, 2) = u[2];
	transform(2, 0) = -(*f)[0];
	transform(2, 1) = -(*f)[1];
	transform(2, 2) = -(*f)[2];

	// Create look at matrix
	return new Matrix(transform * shift);
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
	Matrix *perspective = new Matrix(4, 4);
	perspective->clear();

	float f = 1.0f / std::tan(fovy / 2.0f);

	(*perspective)(0, 0) = f / ar;
	(*perspective)(1, 1) = f;
	(*perspective)(2, 2) = (zfar + znear) / (znear - zfar);
	(*perspective)(2, 3) = (2.0f * zfar * znear) / (znear - zfar);
	(*perspective)(3, 2) = -1.0f;

	return perspective;
}

/**
 * Creates a rotation matrix around the x axis
 *
 * @param angle		the angle in radians by which the matrix should rotate
 * @result			the created rotation matrix
 */
API Matrix *createRotationMatrixX(double angle)
{
	Matrix *rotation = new Matrix(4, 4);
	rotation->identity();

	float c = std::cos(angle);
	float s = std::sin(angle);

	(*rotation)(1, 1) = c;
	(*rotation)(1, 2) = s;
	(*rotation)(2, 1) = -s;
	(*rotation)(2, 2) = c;

	return rotation;
}

/**
 * Creates a rotation matrix around the y axis
 *
 * @param angle		the angle in radians by which the matrix should rotate
 * @result			the created rotation matrix
 */
API Matrix *createRotationMatrixY(double angle)
{
	Matrix *rotation = new Matrix(4, 4);
	rotation->identity();

	float c = std::cos(angle);
	float s = std::sin(angle);

	(*rotation)(0, 0) = c;
	(*rotation)(0, 2) = -s;
	(*rotation)(2, 0) = s;
	(*rotation)(2, 2) = c;

	return rotation;
}

/**
 * Creates a rotation matrix around the z axis
 *
 * @param angle		the angle in radians by which the matrix should rotate
 * @result			the created rotation matrix
 */
API Matrix *createRotationMatrixZ(double angle)
{
	Matrix *rotation = new Matrix(4, 4);
	rotation->identity();

	float c = std::cos(angle);
	float s = std::sin(angle);

	(*rotation)(0, 0) = c;
	(*rotation)(0, 1) = s;
	(*rotation)(1, 0) = -s;
	(*rotation)(1, 1) = c;

	return rotation;
}

/**
 * Creates a rotation matrix around the a general axis
 *
 * @param axis		the axis around with to rotate - as a side-effect, this vector is normalized
 * @param angle		the angle in radians by which the matrix should rotate
 * @result			the created rotation matrix
 */
API Matrix *createRotationMatrix(Vector *axis, double angle)
{
	assert(axis->getSize() == 3);
	axis->normalize();

	float c = std::cos(angle);
	float s = std::sin(angle);
	float C = 1 - c;
	float x = (*axis)[0];
	float y = (*axis)[1];
	float z = (*axis)[2];
	float xs = x * s;
	float ys = y * s;
	float zs = z * s;
	float xC = x * C;
	float yC = y * C;
	float zC = z * C;
	float xyC = x * yC;
	float yzC = y * zC;
	float zxC = z * xC;

	Matrix *rotation = new Matrix(4, 4);
	rotation->identity();

	(*rotation)(0, 1) = xyC - zs;
	(*rotation)(0, 0) = x * xC + c;
	(*rotation)(0, 2) = zxC + ys;
	(*rotation)(1, 0) = xyC + zs;
	(*rotation)(1, 1) = y * yC + c;
	(*rotation)(1, 2) = yzC - xs;
	(*rotation)(2, 0) = zxC - ys;
	(*rotation)(2, 1) = yzC + xs;
	(*rotation)(2, 2) = z * zC + c;

	return rotation;
}
