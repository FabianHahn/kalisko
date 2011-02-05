/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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

#include <cmath>
#include <cassert>
#include "dll.h"

extern "C" {
#include "modules/store/store.h"
#include "modules/store/path.h"
}

#include "api.h"
#include "store.h"
#include "Vector.h"
#include "Matrix.h"

/**
 * Converts a vector to a store
 *
 * @param vector		the vector to convert
 * @result				the created store
 */
API Store *convertVectorToStore(Vector *vector)
{
	Store *store = $(Store *, store, createStoreListValue)(NULL);
	GQueue *list = store->content.list;

	for(unsigned int i = 0; i < vector->getSize(); i++) {
		g_queue_push_head(list, $(Store *, store, createStoreFloatNumberValue)((*vector)[i]));
	}

	return store;
}

/**
 * Converts a matrix to a store
 *
 * @param matrix		the matrix to convert
 * @result				the created store
 */
API Store *convertMatrixToStore(Matrix *matrix)
{
	Store *store = $(Store *, store, createStoreListValue)(NULL);
	GQueue *list = store->content.list;

	for(unsigned int i = 0; i < matrix->getRows(); i++) {
		Store *row = $(Store *, store, createStoreListValue)(NULL);
		GQueue *rowlist = row->content.list;

		for(unsigned int j = 0; j < matrix->getCols(); j++) {
			g_queue_push_head(rowlist, $(Store *, store, createStoreFloatNumberValue)((*matrix)(i, j)));
		}

		g_queue_push_head(list, row);
	}

	return store;
}

/**
 * Converts a store to a vector
 *
 * @param store			the store to convert to a vector
 * @result				the created vector
 */
API Vector *convertStoreToVector(Store *store)
{
	if(store->type != STORE_LIST) {
		LOG_WARNING("Tried to convert non-list store to vector, returning zero vector");
		return new Vector(0);
	}

	GQueue *list = store->content.list;
	Vector *vector = new Vector(g_queue_get_length(list));
	int i = 0;
	for(GList *iter = list->head; iter != NULL; iter = iter->next, i++) {
		Store *element = (Store *) iter->data;

		if(element->type == STORE_FLOAT_NUMBER) {
			(*vector)[i] = element->content.float_number;
		} else {
			LOG_WARNING("Encountered non-float when converting store to vector, setting element to 0");
			(*vector)[i] = 0.0f;
		}
	}

	return vector;
}

/**
 * Converts a store to a matrix
 *
 * @param store			the store to convert to a matrix
 * @result				the created matrix
 */
API Matrix *convertStoreToMatrix(Store *store)
{
	if(store->type != STORE_LIST) {
		LOG_WARNING("Tried to convert non-list store to matrix, returning zero matrix");
		return new Matrix(0, 0);
	}

	GQueue *rowlist = store->content.list;
	Matrix *matrix = NULL;

	if(g_queue_get_length(rowlist) > 0) {
		Store *firstrow = (Store *) rowlist->head->data;
		if(firstrow->type == STORE_LIST) {
			matrix = new Matrix(g_queue_get_length(rowlist), g_queue_get_length(firstrow->content.list));
		}
	}

	if(matrix == NULL) {
		LOG_WARNING("Failed to convert non-list-of-lists store to matrix, returning zero matrix");
		return new Matrix(0, 0);
	}

	int i = 0;
	for(GList *rowiter = rowlist->head; rowiter != NULL; rowiter = rowiter->next, i++) {
		Store *row = (Store *) rowiter->data;

		if(row->type == STORE_LIST) {
			GQueue *collist = row->content.list;
			int j = 0;
			for(GList *coliter = collist->head; coliter != NULL; coliter = coliter->next, j++) {
				Store *element = (Store *) coliter->data;
				if(element->type == STORE_FLOAT_NUMBER) {
					(*matrix)(i, j) = element->content.float_number;
				} else {
					LOG_WARNING("Encountered non-float when converting store to matrix, setting element to 0");
					(*matrix)(i, j) = 0.0f;
				}
			}
		} else {
			LOG_WARNING("Encountered non-list row when converting store to matrix, setting row to 0");
			for(unsigned int j = 0; j < matrix->getCols(); j++) {
				(*matrix)(i, j) = 0.0f;
			}
		}
	}

	return matrix;
}
