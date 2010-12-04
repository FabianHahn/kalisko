extern "C" {
#include <glib.h>
}

#include <string>
#include <sstream>
#include <cmath>
#include "dll.h"
#include "api.h"
#include "Matrix.h"

Matrix::Matrix(unsigned int r, unsigned int c) :
	rows(r), cols(c)
{
	assert(rows > 0 && cols > 0);
	data = new float[rows * cols];
}

Matrix::Matrix(const Matrix& other) :
	rows(other.getRows()), cols(other.getCols())
{
	data = new float[rows * cols];

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) = other(i, j);
		}
	}
}

Matrix::~Matrix()
{
	delete[] data;
}

Matrix& Matrix::operator=(const Matrix& from)
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

Matrix& Matrix::clear()
{
	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) = 0.0f;
		}
	}

	return *this;
}

Matrix& Matrix::identity()
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

Matrix Matrix::transpose() const
{
	Matrix result(cols, rows);

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			result(j, i) = (*this)(i, j);
		}
	}

	return result;
}

Matrix Matrix::operator+(const Matrix& other) const
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

Matrix& Matrix::operator+=(const Matrix& other)
{
	assert(rows == other.getRows() && cols == other.getCols());

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) += other(i, j);
		}
	}

	return *this;
}

Matrix Matrix::operator-(const Matrix& other) const
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

Matrix& Matrix::operator-=(const Matrix& other)
{
	assert(rows == other.getRows() && cols == other.getCols());

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) -= other(i, j);
		}
	}

	return *this;
}

Matrix Matrix::operator*(const Matrix& other) const
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

Vector Matrix::operator*(Vector vector) const
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

Matrix Matrix::operator*(float factor) const
{
	Matrix result(rows, cols);

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			result(i, j) = factor * (*this)(i, j);
		}
	}

	return result;
}

Matrix& Matrix::operator*=(float factor)
{
	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) *= factor;
		}
	}

	return *this;
}

Matrix Matrix::operator/(float factor) const
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

Matrix& Matrix::operator/=(float factor)
{
	assert(factor != 0.0);

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) /= factor;
		}
	}

	return *this;
}

bool Matrix::operator==(const Matrix& other) const
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

std::ostream& operator<<(std::ostream& stream, const Matrix& matrix)
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
