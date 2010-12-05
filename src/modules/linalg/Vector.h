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
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <cmath>

class Vector
{
	public:
		Vector(unsigned int n) :
			size(n)
		{
			assert(size > 0);
			data = new float[size];
		}

		Vector(const Vector& other) :
			size(other.getSize())
		{
			data = new float[size];

			for(unsigned int i = 0; i < size; i++) {
				data[i] = other[i];
			}
		}

		~Vector()
		{
			delete[] data;
		}

		Vector& operator=(const Vector& from)
		{
			if(this == &from) {
				return *this;
			}

			assert(size <= from.getSize());

			for(unsigned int i = 0; i < size; i++) {
				data[i] = from[i];
			}

			return *this;
		}

		Vector& clear()
		{
			for(unsigned int i = 0; i < size; i++) {
				data[i] = 0.0f;
			}

			return *this;
		}

		Vector& normalize()
		{
			float length = getLength();

			// assert(length > 0.0);

			if(length == 0) {
				return *this;
			}

			return (*this) /= length;
		}

		Vector& homogenize()
		{
			if(data[size - 1] == 0.0f) {
				return *this;
			}

			return (*this) /= data[size - 1];
		}

		float getLength2() const
		{
			float ss = 0.0f;

			for(unsigned int i = 0; i < size; i++) {
				ss += data[i] * data[i];
			}

			return ss;
		}

		float getLength() const
		{
			return std::sqrt(getLength2());
		}

		Vector operator+(const Vector& other) const
		{
			assert(size == other.getSize());

			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				result[i] = data[i] + other[i];
			}

			return result;
		}

		Vector& operator+=(const Vector& other)
		{
			assert(size == other.getSize());

			for(unsigned int i = 0; i < size; i++) {
				data[i] += other[i];
			}

			return *this;
		}

		Vector operator-(const Vector& other) const
		{
			assert(size == other.getSize());

			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				result[i] = data[i] - other[i];
			}

			return result;
		}

		Vector& operator-=(const Vector& other)
		{
			assert(size == other.getSize());

			for(unsigned int i = 0; i < size; i++) {
				data[i] -= other[i];
			}

			return *this;
		}

		float operator*(const Vector& other) const
		{
			unsigned int minsize = std::min(size, other.getSize());

			float dot = 0.0f;

			for(unsigned int i = 0; i < minsize; i++) {
				dot += data[i] * other[i];
			}

			return dot;
		}

		Vector operator%(const Vector& other) const
		{
			assert(size >= 3 && other.getSize() >= 3);

			Vector result = Vector(3);
			result[0] = data[1] * other[2] - data[2] * other[1];
			result[1] = data[2] * other[0] - data[0] * other[2];
			result[2] = data[0] * other[1] - data[1] * other[0];

			return result;
		}

		Vector& operator%=(const Vector& other)
		{
			assert(size == 3 && size == other.getSize());

			Vector result = *this % other;
			data[0] = result[0];
			data[1] = result[1];
			data[2] = result[2];

			return *this;
		}

		Vector operator*(float factor) const
		{
			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				result[i] = factor * data[i];
			}

			return result;
		}

		Vector& operator*=(float factor)
		{
			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				data[i] *= factor;
			}

			return *this;
		}

		Vector operator/(float factor) const
		{
			assert(factor != 0.0);

			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				result[i] = data[i] / factor;
			}

			return result;
		}

		Vector& operator/=(float factor)
		{
			assert(factor != 0.0);

			Vector result = Vector(size);

			for(unsigned int i = 0; i < size; i++) {
				data[i] /= factor;
			}

			return *this;
		}

		bool operator==(const Vector& other) const
		{
			assert(size == 3 && size == other.getSize());

			for(unsigned int i = 0; i < size; i++) {
				if(data[i] != other[i]) {
					return false;
				}
			}

			return true;
		}

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

inline std::ostream& operator<<(std::ostream& stream, const Vector& vector)
{
	stream << "[";

	for(unsigned int i = 0; i < vector.getSize(); i++) {
		if(i != 0) {
			stream << "\t";
		}

		stream << vector[i];
	}

	return stream << "]" << std::endl;
}

inline Vector Vector2(float x, float y)
{
	Vector result(2);

	result[0] = x;
	result[1] = y;

	return result;
}

inline Vector Vector3(float x, float y, float z)
{
	Vector result(3);

	result[0] = x;
	result[1] = y;
	result[2] = z;

	return result;
}

inline Vector Vector4(float x, float y, float z, float w)
{
	Vector result(4);

	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;

	return result;
}

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
