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
	return *vector1 == *vector2;
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
