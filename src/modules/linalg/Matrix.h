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
#include <string>
#include <sstream>
#include <cassert>
#include <cmath>

class Matrix
{
	public:
		Matrix(unsigned int r, unsigned int c) :
			rows(r), cols(c)
		{
			assert(rows > 0 && cols > 0);
			data = new float[rows * cols];
		}

		Matrix(const Matrix& other) :
			rows(other.getRows()), cols(other.getCols())
		{
			data = new float[rows * cols];

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) = other(i, j);
				}
			}
		}

		~Matrix()
		{
			delete[] data;
		}

		Matrix& operator=(const Matrix& from)
		{
			if(this == &from) {
				return *this;
			}

			assert(rows == from.getRows() && cols == from.getCols());

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) = from(i, j);
				}
			}

			return *this;
		}

		Matrix& clear()
		{
			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) = 0.0f;
				}
			}

			return *this;
		}

		Matrix& identity()
		{
			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					if(i == j) {
						(*this)(i, j) = 1.0f;
					} else {
						(*this)(i, j) = 0.0f;
					}
				}
			}

			return *this;
		}

		Matrix transpose() const
		{
			Matrix result(cols, rows);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					result(j, i) = (*this)(i, j);
				}
			}

			return result;
		}

		Matrix operator+(const Matrix& other) const
		{
			assert(rows == other.getRows() && cols == other.getCols());

			Matrix result(cols, rows);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					result(i, j) = (*this)(i, j) + other(i, j);
				}
			}

			return result;
		}

		Matrix& operator+=(const Matrix& other)
		{
			assert(rows == other.getRows() && cols == other.getCols());

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) += other(i, j);
				}
			}

			return *this;
		}

		Matrix operator-(const Matrix& other) const
		{
			assert(rows == other.getRows() && cols == other.getCols());

			Matrix result(cols, rows);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					result(i, j) = (*this)(i, j) - other(i, j);
				}
			}

			return result;
		}

		Matrix& operator-=(const Matrix& other)
		{
			assert(rows == other.getRows() && cols == other.getCols());

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) -= other(i, j);
				}
			}

			return *this;
		}

		Matrix operator*(const Matrix& other) const
		{
			assert(cols == other.getRows());

			Matrix result(rows, other.getCols());

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < other.getCols(); j++) {
					float sum = 0.0;
					for(unsigned int k = 0; k < cols; k++) {
						sum += (*this)(i, k) * other(k, j);
					}
					result(i, j) = sum;
				}
			}

			return result;
		}

		Vector operator*(Vector vector) const
		{
			assert(cols >= vector.getSize());

			Vector result(rows);

			for(unsigned int i = 0; i < rows; i++) {
				result[i] = 0.0;

				for(unsigned int j = 0; j < vector.getSize(); j++) {
					result[i] += (*this)(i, j) * vector[j];
				}

				// Assume all other vector dimensions are 1
				for(unsigned int j = vector.getSize(); j < cols; j++) {
					result[i] += (*this)(i, j);
				}
			}

			return result;
		}

		Matrix operator*(float factor) const
		{
			Matrix result(rows, cols);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					result(i, j) = factor * (*this)(i, j);
				}
			}

			return result;
		}

		Matrix& operator*=(float factor)
		{
			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) *= factor;
				}
			}

			return *this;
		}

		Matrix operator/(float factor) const
		{
			assert(factor != 0.0);

			Matrix result(rows, cols);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					result(i, j) = (*this)(i, j) / factor;
				}
			}

			return result;
		}

		Matrix& operator/=(float factor)
		{
			assert(factor != 0.0);

			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					(*this)(i, j) /= factor;
				}
			}

			return *this;
		}

		bool operator==(const Matrix& other) const
		{
			for(unsigned int i = 0; i < rows; i++) {
				for(unsigned int j = 0; j < cols; j++) {
					if((*this)(i, j) != other(i, j)) {
						return false;
					}
				}
			}

			return true;
		}

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

inline std::ostream& operator<<(std::ostream& stream, const Matrix& matrix)
{
	stream << "[";

	for(unsigned int i = 0; i < matrix.getRows(); i++) {
		if(i != 0) {
			stream << " ";
		}

		for(unsigned int j = 0; j < matrix.getCols(); j++) {
			if(j != 0) {
				stream << "\t";
			}

			stream << matrix(i, j);
		}

		if(i != matrix.getRows() - 1) {
			stream << std::endl;
		}
	}

	return stream << "]" << std::endl;
}

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
API Vector *multiplyMatrixVector(Matrix *matrix, Vector *vector);
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
