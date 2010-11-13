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
API void multiplyMatrixScalar(Matrix *matrix, float scalar);
API void divideMatrixScalar(Matrix *matrix, float scalar);
API bool matrixEquals(Matrix *matrix1, Matrix *matrix2);
API float getMatrix(Matrix *matrix, int i, int j);
API void setMatrix(Matrix *matrix, int i, int j, float value);
API unsigned int getMatrixRows(Matrix *matrix);
API unsigned int getMatrixCols(Matrix *matrix);
API GString *dumpMatrix(Matrix *matrix);
API float *getMatrixData(Matrix *matrix);

#ifdef __cplusplus
}
#endif

#endif
