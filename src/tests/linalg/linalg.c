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
#include "api.h"

TEST_CASE(matrix_multiply);

MODULE_NAME("test_linalg");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the linalg module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("linalg", 0, 1, 8));

TEST_SUITE_BEGIN(linalg)
	TEST_CASE_ADD(matrix_multiply);
TEST_SUITE_END

TEST_CASE(matrix_multiply)
{
	Matrix *identity = $(Matrix *, linalg, createMatrix)(3, 3);
	$(void, linalg, eyeMatrix)(identity);

	Matrix *testMatrix = $(Matrix *, linalg, createMatrix)(3, 3);
	$(void, linalg, clearMatrix)(testMatrix);
	$(void, linalg, setMatrix)(testMatrix, 0, 0, 1);
	$(void, linalg, setMatrix)(testMatrix, 0, 1, 2);
	$(void, linalg, setMatrix)(testMatrix, 0, 2, 3);
	$(void, linalg, setMatrix)(testMatrix, 1, 1, -1);
	$(void, linalg, setMatrix)(testMatrix, 2, 2, -5);

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


	TEST_PASS;
}
