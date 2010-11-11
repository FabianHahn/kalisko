#include <cmath>
#include "Matrix.h"

Matrix::Matrix(unsigned int r, unsigned int c) :
	rows(r), cols(c)
{
	assert(rows > 0 && cols > 0);
	data = new double[rows * cols];
}

Matrix::Matrix(const Matrix& other) :
	rows(other.getRows()), cols(other.getCols())
{
	data = new double[rows * cols];

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
			(*this)(i, j) = 0.0;
		}
	}

	return *this;
}

Matrix& Matrix::identity()
{
	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			if(i == j) {
				(*this)(i, j) = 1.0;
			} else {
				(*this)(i, j) = 0.0;
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
			double sum = 0.0;
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
	assert(cols == vector.getSize());

	Vector result(rows);

	for(unsigned int i = 0; i < rows; i++) {
		result[i] = 0.0;

		for(unsigned int j = 0; j < cols; j++) {
			result[i] += (*this)(i, j) * vector[j];
		}
	}

	return result;
}

Matrix Matrix::operator*(double factor) const
{
	Matrix result(rows, cols);

	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			result(i, j) = factor * (*this)(i, j);
		}
	}

	return result;
}

Matrix& Matrix::operator*=(double factor)
{
	for(unsigned int i = 0; i < rows; i++) {
		for(unsigned int j = 0; j < cols; j++) {
			(*this)(i, j) *= factor;
		}
	}

	return *this;
}

Matrix Matrix::operator/(double factor) const
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

Matrix& Matrix::operator/=(double factor)
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
