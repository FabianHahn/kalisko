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


#ifndef LINALG_MATRIX_H
#define LINALG_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif
#include <glib.h>
#ifdef __cplusplus
}
#endif

#include "Vector.h"

#ifdef __cplusplus
class Matrix
{
	public:
		Matrix(unsigned int r, unsigned int c);
		Matrix(const Matrix& other);
		virtual ~Matrix();

		Matrix& operator=(const Matrix& from);

		Matrix& clear();
		Matrix& identity();
		Matrix transpose() const;

		Matrix operator+(const Matrix& other) const;
		Matrix& operator+=(const Matrix& other);
		Matrix operator-(const Matrix& other) const;
		Matrix& operator-=(const Matrix& other);

		Matrix operator*(const Matrix& other) const;
		Vector operator*(Vector vector) const;

		Matrix operator*(float factor) const;
		Matrix& operator*=(float factor);
		Matrix operator/(float factor) const;
		Matrix& operator/=(float factor);

		bool operator==(const Matrix& other) const;

		float& operator()(unsigned int i, unsigned int j)
		{
			assert(i < rows && j < cols);
			return data[i * cols + j];
		}

		const float& operator()(unsigned int i, unsigned int j) const
		{
			assert(i < rows && j < cols);
			return data[i * cols + j];
		}

		unsigned int getRows() const
		{
			return rows;
		}

		unsigned int getCols() const
		{
			return cols;
		}

		float *getData()
		{
			return data;
		}

	private:
		unsigned int rows;
		unsigned int cols;
		float *data;
};

inline Matrix operator*(float factor, const Matrix& matrix)
{
	return matrix * factor;
}

std::ostream& operator<<(std::ostream& stream, const Matrix& matrix);

#else
typedef struct Matrix Matrix;
#endif

#ifdef __cplusplus
extern "C" {
#endif

API Matrix *createMatrix(unsigned int r, unsigned int c);
API void assignMatrix(Matrix *target, Matrix *source);
API Matrix *copyMatrix(Matrix *other);
API void freeMatrix(Matrix *matrix);
API void clearMatrix(Matrix *matrix);
API void eyeMatrix(Matrix *matrix);
API Matrix *transposeMatrix(Matrix *matrix);
API void addMatrix(Matrix *matrix, Matrix *other);
API Matrix *sumMatrices(Matrix *matrix1, Matrix *matrix2);
API void subtractMatrix(Matrix *matrix, Matrix *other);
API Matrix *diffMatrices(Matrix *matrix1, Matrix *matrix2);
API Matrix *multiplyMatrices(Matrix *matrix1, Matrix *matrix2);
API Vector *multiplyMatrixWithVector(Matrix *matrix, Vector *vector);
API void multiplyMatrixScalar(Matrix *matrix, double scalar);
API void divideMatrixScalar(Matrix *matrix, double scalar);
API bool matrixEquals(Matrix *matrix1, Matrix *matrix2);
API float getMatrix(Matrix *matrix, int i, int j);
API void setMatrix(Matrix *matrix, int i, int j, double value);
API unsigned int getMatrixRows(Matrix *matrix);
API unsigned int getMatrixCols(Matrix *matrix);
API GString *dumpMatrix(Matrix *matrix);
API float *getMatrixData(Matrix *matrix);

#ifdef __cplusplus
}
#endif

#endif
