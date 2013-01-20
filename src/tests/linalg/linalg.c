/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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

#include <glib.h>
#include <string.h>

#include "dll.h"
#include "test.h"
#include "memory_alloc.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#define API

TEST_CASE(matrix_matrix_multiplication);
TEST_CASE(matrix_vector_multiplication);
TEST_CASE(vector_vector_multiplication);

MODULE_NAME("test_linalg");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the linalg module");
MODULE_VERSION(0, 1, 4);
MODULE_BCVERSION(0, 1, 4);
MODULE_DEPENDS(MODULE_DEPENDENCY("linalg", 0, 2, 6));

TEST_SUITE_BEGIN(linalg)
	TEST_CASE_ADD(matrix_matrix_multiplication);
	TEST_CASE_ADD(matrix_vector_multiplication);
	TEST_CASE_ADD(vector_vector_multiplication);
TEST_SUITE_END

static Matrix *getTestMatrix();

TEST_CASE(matrix_matrix_multiplication)
{
	Matrix *identity = $(Matrix *, linalg, createMatrix)(3, 3);
	$(void, linalg, eyeMatrix)(identity);

	Matrix *testMatrix = getTestMatrix();

	// Multiply with identity
	Matrix *result = $(Matrix *, linalg, multiplyMatrices)(identity, testMatrix);
	TEST_ASSERT($(bool, linalg, matrixEquals)(result, testMatrix));
	$(void, linalg, freeMatrix)(result);

	// Multiply with itself
	result = $(Matrix *, linalg, multiplyMatrices)(testMatrix, testMatrix);
	Matrix *solution = $(Matrix *, linalg, createMatrix)(3, 3);
	$(void, linalg, eyeMatrix)(solution);
	$(void, linalg, setMatrix)(solution, 0, 2, -12);
	$(void, linalg, setMatrix)(solution, 1, 2, 0);
	$(void, linalg, setMatrix)(solution, 2, 2, 25);
	TEST_ASSERT($(bool, linalg, matrixEquals)(result, solution));
	$(void, linalg, freeMatrix)(solution);
	$(void, linalg, freeMatrix)(result);

	// Clean up
	$(void, linalg, freeMatrix)(identity);
	$(void, linalg, freeMatrix)(testMatrix);
}

TEST_CASE(matrix_vector_multiplication)
{
	Matrix *testMatrix = getTestMatrix();
	Vector *testVector = $(Vector *, linalg, createVector)(3);
	$(void, linalg, setVector)(testVector, 0, -1);
	$(void, linalg, setVector)(testVector, 1, 3.14);
	$(void, linalg, setVector)(testVector, 2, 0.5);

	Vector *result = $(Vector *, linalg, multiplyMatrixVector)(testMatrix, testVector);
	Vector *solution = $(Vector *, linalg, createVector)(3);
	$(void, linalg, setVector)(solution, 0, 6.78);
	$(void, linalg, setVector)(solution, 1, -3.14);
	$(void, linalg, setVector)(solution, 2, -2.5);
	TEST_ASSERT($(bool, linalg, vectorEquals)(result, solution));

	$(void, linalg, freeMatrix)(testMatrix);
	$(void, linalg, freeVector)(testVector);
	$(void, linalg, freeVector)(result);
	$(void, linalg, freeVector)(solution);
}

TEST_CASE(vector_vector_multiplication)
{
	Vector *testVector1 = $(Vector *, linalg, createVector)(3);
	$(void, linalg, setVector)(testVector1, 0, -1);
	$(void, linalg, setVector)(testVector1, 1, 27);
	$(void, linalg, setVector)(testVector1, 2, 0.5);

	Vector *testVector2 = $(Vector *, linalg, createVector)(3);
	$(void, linalg, setVector)(testVector2, 0, 5.0);
	$(void, linalg, setVector)(testVector2, 1, 0.0);
	$(void, linalg, setVector)(testVector2, 2, 1.0);

	float dotResult = $(float, linalg, dotVectors)(testVector1, testVector2);
	TEST_ASSERT(dotResult == -4.5);

	Vector *crossVector = $(Vector *, linalg, crossVectors)(testVector1, testVector2);
	Vector *crossVectorSolution = $(Vector *, linalg, createVector)(3);
	$(void, linalg, setVector)(crossVectorSolution, 0, 27.0);
	$(void, linalg, setVector)(crossVectorSolution, 1, 3.5);
	$(void, linalg, setVector)(crossVectorSolution, 2, -135.0);
	TEST_ASSERT($(bool, linalg, vectorEquals)(crossVector, crossVectorSolution));

	$(void, linalg, freeVector)(testVector1);
	$(void, linalg, freeVector)(testVector2);
	$(void, linalg, freeVector)(crossVector);
	$(void, linalg, freeVector)(crossVectorSolution);
}

static Matrix *getTestMatrix()
{
	Matrix *testMatrix = $(Matrix *, linalg, createMatrix)(3, 3);
	$(void, linalg, clearMatrix)(testMatrix);
	$(void, linalg, setMatrix)(testMatrix, 0, 0, 1);
	$(void, linalg, setMatrix)(testMatrix, 0, 1, 2);
	$(void, linalg, setMatrix)(testMatrix, 0, 2, 3);
	$(void, linalg, setMatrix)(testMatrix, 1, 1, -1);
	$(void, linalg, setMatrix)(testMatrix, 2, 2, -5);

	return testMatrix;
}
