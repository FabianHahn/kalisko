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
#include <stdio.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"

#define API
#include "table.h"

MODULE_NAME("table");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module containing a basic table representation");
MODULE_VERSION(0, 1, 9);
MODULE_BCVERSION(0, 1, 3);
MODULE_NODEPS;

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

API Table *newTable()
{
	return newTableFull(MODULE_TABLE_DEFAULT_ALLOC_ROWS, MODULE_TABLE_DEFAULT_ALLOC_COLS);
}

API Table *newTableFull(int preAllocRows, int preAllocCols)
{
	// create table in memory
	Table *table = ALLOCATE_OBJECT(Table);
	table->freeTableCallback = NULL;
	table->newCellCallback = NULL;
	table->outputGeneratorCallback = NULL;
	table->table = NULL;
	table->tag = NULL;
	table->copyCellCallback = NULL;
	table->rows = 0;
	table->cols = 0;
	table->freeRowsAmount = preAllocRows;
	table->freeColsAmount = preAllocCols;

	// speed up the return, we do not need to do the rest
	if(preAllocRows == 0 && preAllocCols == 0) {
		return table;
	}

	// allocate the space we need
	table->table = ALLOCATE_OBJECTS(TableCell **, preAllocRows);

	for(int row = 0; row < preAllocRows; row++) {
		table->table[row] = ALLOCATE_OBJECTS(TableCell *, preAllocCols);
		memset(table->table[row], 0, sizeof(TableCell *) * preAllocCols);
	}

	return table;
}

API TableCell *newTableCell(Table *table)
{
	TableCell *cell = ALLOCATE_OBJECT(TableCell);
	cell->content = NULL;
	cell->freeCellCallback = NULL;
	cell->tag = NULL;
	cell->freeContent = false;

	if(table->newCellCallback) {
		table->newCellCallback(table, cell);
	}

	return cell;
}

API void freeTable(Table *table)
{
	int rowCount = table->rows + table->freeRowsAmount;
	int colCount = table->cols + table->freeColsAmount;

	for(int iRow = 0; iRow < rowCount; iRow++) {
		for(int iCol = 0; iCol < colCount; iCol++) {
			TableCell *cell = table->table[iRow][iCol];
			if(cell != NULL) {
				freeCell(cell);
			}
		}

		free(table->table[iRow]);
	}

	if(table->table != NULL) {
		free(table->table);
	}

	if(table->freeTableCallback != NULL) {
		table->freeTableCallback(table);
	}

	free(table);
}

API void freeCell(TableCell *cell)
{
	if(cell->freeCellCallback != NULL) {
		cell->freeCellCallback(cell);
	}

	if(cell->freeContent) {
		free(cell->content);
	}

	free(cell);
}

API int appendTableCol(Table *table, int colAmount, TableCell *cellTemplate)
{
	if(colAmount == 0) {
		return -1;
	}

	if(table->table == NULL || table->rows == 0) {
		appendTableRow(table, 1, cellTemplate);
	}

	int colCount = table->cols + colAmount;
	int colCountToAlloc = colAmount - table->freeColsAmount;

	// Allocated the extra space if needed
	if(colCountToAlloc > 0) {
		for(int row = 0; row < table->freeRowsAmount + table->rows; row++) {
			// Create the space to store new column(s)
			table->table[row] = REALLOCATE_OBJECT(TableCell *, table->table[row], sizeof(TableCell *) * colCount);
			memset(table->table[row], 0, sizeof(TableCell *) * colCount);
		}

		table->freeColsAmount = 0; // we used all up
	} else {
		table->freeColsAmount -= colAmount;
	}

	// Fill the space
	for(int row = 0; row < table->rows; row++) {
		// Fill rows with new TableCells for the new column
		for(int col = table->cols; col < colCount; col++) {
			if(cellTemplate == NULL) {
				table->table[row][col] = newTableCell(table);
			} else {
				table->table[row][col] = copyTableCell(table, cellTemplate);
			}
		}
	}

	int firstIndex = table->cols;
	table->cols = table->cols + colAmount;

	return firstIndex;
}

API int appendTableRow(Table *table, int rowAmount, TableCell *cellTemplate)
{
	if(rowAmount == 0) {
		return -1;
	}

	int rowCount = table->rows + rowAmount;
	int rowCountToAlloc = rowAmount - table->freeRowsAmount;
	int startNotPreAllocedRows = table->rows + table->freeRowsAmount - 1;

	// Allocate the extra needed space
	if(rowCountToAlloc > 0) {
		if(table->table == NULL) {
			table->table = ALLOCATE_OBJECTS(TableCell **, rowCount);
			memset(table->table, 0, sizeof(TableCell **) * rowCount);
		} else {
			TableCell ***extendedTable = REALLOCATE_OBJECT(TableCell **, table->table, sizeof(TableCell **) * rowCount);
			table->table = extendedTable;
		}

		table->freeRowsAmount = 0; // we used all up
	} else {
		table->freeRowsAmount -= rowAmount;
	}

	// Fill new rows with TableCells
	int allocCols = table->cols + table->freeColsAmount;
	for(int row = table->rows; row < rowCount; row++) {
		if(row > startNotPreAllocedRows && allocCols > 0) {
			table->table[row] = ALLOCATE_OBJECTS(TableCell *, allocCols);
		}

		for(int col = 0; col < table->cols; col++) {
			if(cellTemplate == NULL) {
				table->table[row][col] = newTableCell(table);
			} else {
				table->table[row][col] = copyTableCell(table, cellTemplate);
			}
		}
	}

	int firstIndex = table->rows;
	table->rows = table->rows + rowAmount;

	return firstIndex;
}

API TableCell *copyTableCell(Table *table, TableCell *original)
{
	TableCell *copy = newTableCell(table);
	if(original->content != NULL) {
		copy->content = strdup(original->content);
		copy->freeContent = true;
	}

	if(table->copyCellCallback) {
		table->copyCellCallback(table, original, copy);
	}

	return copy;
}

API bool replaceTableCell(Table *table, TableCell *cell, int row, int col)
{
	// check params
	if(table == NULL) {
		return false;
	}

	if(cell == NULL) {
		return false;
	}

	if(row >= table->rows) {
		return false;
	}

	if(col >= table->cols) {
		return false;
	}

	// remove old cell
	freeCell(table->table[row][col]);

	// set new cell
	table->table[row][col] = cell;

	return true;
}

API char *getTableString(Table *table)
{
	if(table->outputGeneratorCallback != NULL) {
		return table->outputGeneratorCallback(table);
	} else {
		return NULL;
	}
}
