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
#include "Matrix.h"

/**
 * Creates a matrix
 *
 * @param r		the number of rows
 * @param c		the number of columns
 * @result		the created matrix
 */
API Matrix *createMatrix(unsigned int r, unsigned int c)
{
	return new Matrix(r, c);
}

/**
 * Assigns a matrix to another matrix, causing its contents to be copied
 *
 * @param target		the target matrix to which the source matrix should be assigned
 * @param source		the source matrix from which the content should be copied
 */
API void assignMatrix(Matrix *target, Matrix *source)
{
	*target = *source;
}

/**
 * Copies a matrix
 *
 * @param other		the matrix to copy from
 * @result			the copied matrix
 */
API Matrix *copyMatrix(Matrix *other)
{
	return new Matrix(*other);
}

/**
 * Deletes a matrix
 *
 * @param matrix	the matrix to delete
 */
API void freeMatrix(Matrix *matrix)
{
	delete matrix;
}

/**
 * Clears a matrix by setting all its elements to zero
 *
 * @param matrix	the matrix to clear
 */
API void clearMatrix(Matrix *matrix)
{
	matrix->clear();
}

/**
 * Converts a matrix into an identity matrix by setting all diagonal elements to one and all other elements to zero
 *
 * @param matrix	the matrix to convert to identity
 */
API void eyeMatrix(Matrix *matrix)
{
	matrix->identity();
}

/**
 * Transposes a matrix
 *
 * @param matrix	the matrix to transpose
 * @result			the transposed matrix
 */
API Matrix *transposeMatrix(Matrix *matrix)
{
	return new Matrix(matrix->transpose());
}

/**
 * Adds a matrix to another matrix
 *
 * @param matrix	the matrix to which the other matrix should be added
 * @param other		the matrix that should be added
 */
API void addMatrix(Matrix *matrix, Matrix *other)
{
	*matrix += *other;
}

/**
 * Computes the sum of two matrices
 *
 * @param matrix1	the first matrix to sum
 * @param matrix2	the second matrix to sum
 * @result			the sum of the two matrices
 */
API Matrix *sumMatrices(Matrix *matrix1, Matrix *matrix2)
{
	return new Matrix(*matrix1 + *matrix2);
}

/**
 * Subtracts a matrix from another matrix
 *
 * @param matrix	the matrix from which the other matrix should be substracted
 * @param other		the matrix that should be substracted
 */
API void subtractMatrix(Matrix *matrix, Matrix *other)
{
	*matrix -= *other;
}

/**
 * Computes the different of two matrices
 *
 * @param matrix1	the matrix to subtract from
 * @param matrix2	the matrix that should be substracted from the first one
 * @result			the signed difference of the two matrices
 */
API Matrix *diffMatrices(Matrix *matrix1, Matrix *matrix2)
{
	return new Matrix(*matrix1 - *matrix2);
}

/**
 * Multiplies two matrices
 *
 * @param matrix1		the first matrix that should be multiplied
 * @param matrix2		the second matrix that should be multiplied
 * @result				the product of the two matrices
 */
API Matrix *multiplyMatrices(Matrix *matrix1, Matrix *matrix2)
{
	return new Matrix(*matrix1 * *matrix2);
}

/**
 * Computes a matrix-vector product
 *
 * @param matrix		the matrix that should be multiplied with the vector
 * @param vector		the vector that should be multiplied with the matrix
 * @result				the matrix-vector product of the input values
 */
API Vector *multiplyMatrixWithVector(Matrix *matrix, Vector *vector)
{
	return new Vector(*matrix * *vector);
}

/**
 * Multiplies a matrix with a scalar
 *
 * @param matrix		the matrix to multiply with a scalar
 * @param scalar		the scalar to multiply the matrix with
 */
API void multiplyMatrixScalar(Matrix *matrix, double scalar)
{
	*matrix *= scalar;
}

/**
 * Divides a matrix by a scalar
 *
 * @param matrix		the matrix to divide by a scalar
 * @param scalar		the scalar to divide the matrix by
 */
API void divideMatrixScalar(Matrix *matrix, double scalar)
{
	*matrix /= scalar;
}

/**
 * Checks if two matrices are equal
 *
 * @param matrix1		the first matrix to compare
 * @param matrix2		the second matrix to compare
 * @result				true if the matrices are equal
 */
API bool matrixEquals(Matrix *matrix1, Matrix *matrix2)
{
	return matrix1 == matrix2;
}

/**
 * Returns a matrix element
 *
 * @param matrix		the matrix to read
 * @param i				the row of the matrix element to retrieve
 * @param j				the column of the matrix element to retrieve
 * @result				the element at matrix position (i,j)
 */
API float getMatrix(Matrix *matrix, int i, int j)
{
	return (*matrix)(i, j);
}

/**
 * Sets a matrix element
 *
 * @param matrix		the matrix to set
 * @param i				the row of the matrix element to set
 * @param j				the column of the matrix element to set
 * @param value			the new value for the matrix element at position (i,j)
 */
API void setMatrix(Matrix *matrix, int i, int j, double value)
{
	(*matrix)(i, j) = value;
}

/**
 * Returns the number of rows of a matrix
 *
 * @param matrix		the matrix for which the number of rows should be retrieved
 * @result				the number of rows of the matrix
 */
API unsigned int getMatrixRows(Matrix *matrix)
{
	return matrix->getRows();
}

/**
 * Returns the number of columns of a matrix
 *
 * @param matrix		the matrix for which the number of columns should be retrieved
 * @result				the number of columns of the matrix
 */
API unsigned int getMatrixCols(Matrix *matrix)
{
	return matrix->getCols();
}

/**
 * Returns the string representation of a matrix
 *
 * @param matrix	the matrix to be dumped
 * @result			the string representation of the matrix
 */
API GString *dumpMatrix(Matrix *matrix)
{
	std::stringstream stream;
	stream << *matrix;
	std::string outstr = stream.str();
	return g_string_new(outstr.c_str());
}

/**
 * Returns a pointer to the matrix data
 *
 * @param matrix	the matrix for which the data pointer should be returned
 * @result			the data pointer of the matrix
 */
API float *getMatrixData(Matrix *matrix)
{
	return matrix->getData();
}
