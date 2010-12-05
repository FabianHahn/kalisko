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

#ifndef LINALG_VECTOR_H
#define LINALG_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif
#include <glib.h>
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <iostream>
#include <assert.h>

class Vector
{
	public:
		Vector(unsigned int n);
		Vector(const Vector& other);
		virtual ~Vector();

		Vector& operator=(const Vector& from);

		Vector& clear();
		Vector& normalize();
		Vector& homogenize();

		float getLength() const;
		float getLength2() const;

		Vector operator+(const Vector& other) const;
		Vector& operator+=(const Vector& other);
		Vector operator-(const Vector& other) const;
		Vector& operator-=(const Vector& other);
		float operator*(const Vector& other) const;
		Vector operator%(const Vector& other) const;
		Vector& operator%=(const Vector& other);

		Vector operator*(float factor) const;
		Vector& operator*=(float factor);
		Vector operator/(float factor) const;
		Vector& operator/=(float factor);

		bool operator==(const Vector& other) const;

		float& operator[](unsigned int i)
		{
			assert(i < size);
			return data[i];
		}

		const float& operator[](unsigned int i) const
		{
			assert(i < size);
			return data[i];
		}

		unsigned int getSize() const
		{
			return size;
		}

		float *getData()
		{
			return data;
		}

	private:
		unsigned int size;
		float *data;
};

inline Vector operator*(float factor, const Vector& vector)
{
	return vector * factor;
}

std::ostream& operator<<(std::ostream& stream, const Vector& vector);

Vector Vector2(float x, float y);
Vector Vector3(float x, float y, float z);
Vector Vector4(float x, float y, float z, float w);

#else
typedef struct Vector Vector;
#endif

#ifdef __cplusplus
extern "C" {
#endif

API Vector *createVector(unsigned int n);
API void assignVector(Vector *target, Vector *source);
API Vector *copyVector(Vector *vector);
API void freeVector(Vector *vector);
API void clearVector(Vector *vector);
API void normalizeVector(Vector *vector);
API void homogenizeVector(Vector *vector);
API float getVectorLength(Vector *vector);
API float getVectorLength2(Vector *vector);
API void addVector(Vector *vector, Vector *other);
API Vector *sumVectors(Vector *vector1, Vector *vector2);
API void subtractVector(Vector *vector, Vector *other);
API Vector *diffVectors(Vector *vector1, Vector *vector2);
API float dotVectors(Vector *vector1, Vector *vector2);
API Vector *crossVectors(Vector *vector1, Vector *vector2);
API void multiplyVectorScalar(Vector *vector, double scalar);
API void divideVectorScalar(Vector *vector, double scalar);
API bool vectorEquals(Vector *vector1, Vector *vector2);
API float getVector(Vector *vector, int i);
API void setVector(Vector *vector, int i, double value);
API unsigned int getVectorSize(Vector *vector);
API GString *dumpVector(Vector *vector);
API float *getVectorData(Vector *vector);
API Vector *createVector2(double x, double y);
API Vector *createVector3(double x, double y, double z);
API Vector *createVector4(double x, double y, double z, double w);

#ifdef __cplusplus
}
#endif

#endif
