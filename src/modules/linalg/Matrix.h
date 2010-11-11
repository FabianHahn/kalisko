#ifndef LINALG_MATRIX_H
#define LINALG_MATRIX_H

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

		Matrix operator*(double factor) const;
		Matrix& operator*=(double factor);
		Matrix operator/(double factor) const;
		Matrix& operator/=(double factor);

		bool operator==(const Matrix& other) const;

		double& operator()(unsigned int i, unsigned int j)
		{
			assert(i < rows && j < cols);
			return data[i * cols + j];
		}

		const double& operator()(unsigned int i, unsigned int j) const
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

	private:
		unsigned int rows;
		unsigned int cols;
		double *data;
};

inline Matrix operator*(double factor, const Matrix& matrix)
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
API Matrix *copyMatrix(Matrix *other);
API void freeMatrix(Matrix *matrix);

#ifdef __cplusplus
}
#endif

#endif
