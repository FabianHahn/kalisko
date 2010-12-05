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


extern "C" {
#include <glib.h>
}

#include <string>
#include <sstream>

#include <cmath>
#include "dll.h"
#include "api.h"
#include "Vector.h"

Vector::Vector(unsigned int n) :
	size(n)
{
	assert(size > 0);
	data = new float[size];
}

Vector::Vector(const Vector& other) :
	size(other.getSize())
{
	data = new float[size];

	for(unsigned int i = 0; i < size; i++) {
		data[i] = other[i];
	}
}

Vector::~Vector()
{
	delete[] data;
}

Vector& Vector::operator=(const Vector& from)
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

Vector& Vector::clear()
{
	for(unsigned int i = 0; i < size; i++) {
		data[i] = 0.0f;
	}

	return *this;
}

Vector& Vector::normalize()
{
	float length = getLength();

	// assert(length > 0.0);

	if(length == 0) {
		return *this;
	}

	return (*this) /= length;
}

Vector& Vector::homogenize()
{
	if(data[size - 1] == 0.0f) {
		return *this;
	}

	return (*this) /= data[size - 1];
}

float Vector::getLength() const
{
	return std::sqrt(getLength2());
}

float Vector::getLength2() const
{
	float ss = 0.0f;

	for(unsigned int i = 0; i < size; i++) {
		ss += data[i] * data[i];
	}

	return ss;
}

Vector Vector::operator+(const Vector& other) const
{
	assert(size == other.getSize());

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] + other[i];
	}

	return result;
}

Vector& Vector::operator+=(const Vector& other)
{
	assert(size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		data[i] += other[i];
	}

	return *this;
}

Vector Vector::operator-(const Vector& other) const
{
	assert(size == other.getSize());

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] - other[i];
	}

	return result;
}

Vector& Vector::operator-=(const Vector& other)
{
	assert(size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		data[i] -= other[i];
	}

	return *this;
}

float Vector::operator*(const Vector& other) const
{
	unsigned int minsize = std::min(size, other.getSize());

	float dot = 0.0f;

	for(unsigned int i = 0; i < minsize; i++) {
		dot += data[i] * other[i];
	}

	return dot;
}

Vector Vector::operator%(const Vector& other) const
{
	assert(size >= 3 && other.getSize() >= 3);

	Vector result = Vector(3);
	result[0] = data[1] * other[2] - data[2] * other[1];
	result[1] = data[2] * other[0] - data[0] * other[2];
	result[2] = data[0] * other[1] - data[1] * other[0];

	return result;
}

Vector& Vector::operator%=(const Vector& other)
{
	assert(size == 3 && size == other.getSize());

	Vector result = *this % other;
	data[0] = result[0];
	data[1] = result[1];
	data[2] = result[2];

	return *this;
}

Vector Vector::operator*(float factor) const
{
	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = factor * data[i];
	}

	return result;
}

Vector& Vector::operator*=(float factor)
{
	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		data[i] *= factor;
	}

	return *this;
}

Vector Vector::operator/(float factor) const
{
	assert(factor != 0.0);

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] / factor;
	}

	return result;
}

Vector& Vector::operator/=(float factor)
{
	assert(factor != 0.0);

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		data[i] /= factor;
	}

	return *this;
}

bool Vector::operator==(const Vector& other) const
{
	assert(size == 3 && size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		if(data[i] != other[i]) {
			return false;
		}
	}

	return true;
}

std::ostream& operator<<(std::ostream& stream, const Vector& vector)
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

Vector Vector2(float x, float y)
{
	Vector result(2);

	result[0] = x;
	result[1] = y;

	return result;
}

Vector Vector3(float x, float y, float z)
{
	Vector result(3);

	result[0] = x;
	result[1] = y;
	result[2] = z;

	return result;
}

Vector Vector4(float x, float y, float z, float w)
{
	Vector result(4);

	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;

	return result;
}

/**
 * Creates a vector
 *
 * @param n			the number of vector elements
 * @result			the created vector
 */
API Vector *createVector(unsigned int n)
{
	return new Vector(n);
}

/**
 * Assigns a vector to another vector, causing its contents to be copied
 *
 * @param target		the target vector to which the source vector should be assigned
 * @param source		the source vector from which the content should be copied
 */
API void assignVector(Vector *target, Vector *source)
{
	*target = *source;
}

/**
 * Copies a vector
 *
 * @param vector	the vector to be copied
 * @result			the copied vector
 */
API Vector *copyVector(Vector *vector)
{
	return new Vector(*vector);
}

/**
 * Frees a vector
 *
 * @param vector	the vector to be freed
 */
API void freeVector(Vector *vector)
{
	delete vector;
}

/**
 * Clear a vector by setting all its elements to zero
 *
 * @param vector	the vector to be cleared
 */
API void clearVector(Vector *vector)
{
	vector->clear();
}

/**
 * Normalizes a vector
 *
 * @param vector	the vector to be normalized
 */
API void normalizeVector(Vector *vector)
{
	vector->normalize();
}

/**
 * Homogenize a vector by dividing every component by the last component
 *
 * @param vector	the vector to be homogenized
 */
API void homogenizeVector(Vector *vector)
{
	vector->homogenize();
}

/**
 * Computes a vector's length (L2 norm)
 *
 * @param vector		the vector for which the length should be computed
 * @result				the length of the vector
 */
API float getVectorLength(Vector *vector)
{
	return vector->getLength();
}

/**
 * Computes a vector's squared length (scalar product with itself)
 *
 * @param vector		the vector for which the squared length should be computed
 * @result				the squared length of the vector
 */
API float getVectorLength2(Vector *vector)
{
	return vector->getLength2();
}


/**
 * Adds a vector to another vector
 *
 * @param vector	the vector to which the other vector should be added
 * @param other		the vector that should be added
 */
API void addVector(Vector *vector, Vector *other)
{
	*vector += *other;
}

/**
 * Computes the sum of two vectors
 *
 * @param vector1	the first vector to sum
 * @param vector2	the second vector to sum
 * @result			the sum of the two vectors
 */
API Vector *sumVectors(Vector *vector1, Vector *vector2)
{
	return new Vector(*vector1 + *vector2);
}

/**
 * Subtracts a vector from another vector
 *
 * @param vector	the vector from which the other vector should be substracted
 * @param other		the vector that should be substracted
 */
API void subtractVector(Vector *vector, Vector *other)
{
	*vector -= *other;
}

/**
 * Computes the different of two vectors
 *
 * @param vector1	the vector to subtract from
 * @param vector2	the vector that should be substracted from the first one
 * @result			the signed difference of the two vectors
 */
API Vector *diffVectors(Vector *vector1, Vector *vector2)
{
	return new Vector(*vector1 - *vector2);
}

/**
 * Computes the scalar product (dot product) of two vectors of the same size
 *
 * @param vector1	the first factor of the scalar product
 * @param vector2	the second factor of the scalar product
 * @result 			the scalar product of the two vectors
 */
API float dotVectors(Vector *vector1, Vector *vector2)
{
	return *vector1 * *vector2;
}

/**
 * Computes the vector product (cross product) of two vectors of size 3
 *
 * @param vector1	the first factor of the cross product
 * @param vector2	the second factor of the cross product
 * @result 			the cross product of the two vectors
 */
API Vector *crossVectors(Vector *vector1, Vector *vector2)
{
	return new Vector(*vector1 % *vector2);
}

/**
 * Multiplies a vector with a scalar
 *
 * @param vector		the vector to multiply with a scalar
 * @param scalar		the scalar to multiply the vector with
 */
API void multiplyVectorScalar(Vector *vector, double scalar)
{
	*vector *= scalar;
}

/**
 * Divides a vector by a scalar
 *
 * @param vector		the vector to divide by a scalar
 * @param scalar		the scalar to divide the vector by
 */
API void divideVectorScalar(Vector *vector, double scalar)
{
	*vector /= scalar;
}


/**
 * Checks if two vectors are equal
 *
 * @param vector1		the first vector to compare
 * @param vector2		the second vector to compare
 * @result				true if the vectors are equal
 */
API bool vectorEquals(Vector *vector1, Vector *vector2)
{
	return vector1 == vector2;
}


/**
 * Returns a vector element
 *
 * @param vector		the vector to read
 * @param i				the index of the vector element to retrieve
 * @result				the element at vector position i
 */
API float getVector(Vector *vector, int i)
{
	return (*vector)[i];
}

/**
 * Sets a vector element
 *
 * @param vector		the vector to set
 * @param i				the row of the matrix element to set
 * @param value			the new value for the vector element at position i
 */
API void setVector(Vector *vector, int i, double value)
{
	(*vector)[i] = value;
}

/**
 * Returns the size of a vector
 *
 * @param vector		the vector for which to retrieve the size
 * @result				the size of the vector
 */
API unsigned int getVectorSize(Vector *vector)
{
	return vector->getSize();
}

/**
 * Returns the string representation of a vector
 *
 * @param vector	the vector to be dumped
 * @result			the string representation of the vector
 */
API GString *dumpVector(Vector *vector)
{
	std::stringstream stream;
	stream << *vector;
	std::string outstr = stream.str();
	return g_string_new(outstr.c_str());
}

/**
 * Returns a pointer to the vector data
 *
 * @param vector	the vector for which the data pointer should be returned
 * @result			the data pointer of the vector
 */
API float *getVectorData(Vector *vector)
{
	return vector->getData();
}

/**
 * Constructs a 2D vector directly from two elements
 *
 * @param x			the first element of the resulting vector
 * @param y			the second element of the resulting vector
 * @result			a 2-vector with elements (x,y)
 */
API Vector *createVector2(double x, double y)
{
	return new Vector(Vector2(x, y));
}

/**
 * Constructs a 3D vector directly from two elements
 *
 * @param x			the first element of the resulting vector
 * @param y			the second element of the resulting vector
 * @param z			the third element of the resulting vector
 * @result			a 3-vector with elements (x,y,z)
 */
API Vector *createVector3(double x, double y, double z)
{
	return new Vector(Vector3(x, y, z));
}

/**
 * Constructs a 3D vector directly from two elements
 *
 * @param x			the first element of the resulting vector
 * @param y			the second element of the resulting vector
 * @param z			the third element of the resulting vector
 * @param w			the fourth element of the resulting vector
 * @result			a 4-vector with elements (x,y,z,w)
 */
API Vector *createVector4(double x, double y, double z, double w)
{
	return new Vector(Vector4(x, y, z, w));
}
