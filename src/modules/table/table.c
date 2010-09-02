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
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"

#include "api.h"
#include "table.h"

MODULE_NAME("table");
MODULE_AUTHOR("The kalisko team");
MODULE_DESCRIPTION("Module containing a basic table representation");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Returns a newly created Table.
 *
 * @return the new Table
 */
API Table *newTable()
{
	Table *table = ALLOCATE_OBJECT(Table);
	table->cols = 0;
	table->freeTable = NULL;
	table->longestContentLength = 0;
	table->newCell = NULL;
	table->outputGenerator = NULL;
	table->rows = 0;
	table->table = NULL;
	table->tag = NULL;
	table->copyCell = NULL;

	return table;
}

/**
 * Returns a new TableCell for the given table. The cell
 * do not yet belong to table itself.
 *
 * This function is used to create a TableCell which works as
 * a template or internally by Table modules.
 *
 * @see appendTableCol
 * @see appendTableRow
 *
 * @param table		The table for which the TableCell is.
 * @return a new TableCell
 */
API TableCell *newTableCell(Table *table)
{
	TableCell *cell = ALLOCATE_OBJECT(TableCell);
	cell->content = NULL;
	cell->format = 0;
	cell->freeCell = NULL;
	cell->tag = NULL;

	if(table->newCell) {
		table->newCell(table, cell);
	}

	return cell;
}

/**
 * Frees the given table.
 *
 * It calls also the TableCell.freeCell and Table.freeTable functions if
 * they are set.
 *
 * @param table		The table to free
 */
API void freeTable(Table *table)
{
	// free cells
	for(int iCol = 0; iCol < table->cols; iCol++) {
		for(int iRow = 0; iRow < table->rows; iRow++) {
			TableCell *cell = getTableCell(table, iRow, iCol);
			freeCell(cell);
		}
	}

	// free table
	if(table->freeTable) {
		table->freeTable(table);
	}

	if(table->table) {
		free(table->table);
	}

	free(table);
}

/**
 * Frees the given TableCell
 *
 * @param cell	The cell to free
 */
API void freeCell(TableCell *cell)
{
	if(cell->freeCell) {
		cell->freeCell(cell);
	}

	if(cell->content) {
		free(cell->content);
	}

	free(cell);
}

/**
 * Returns the TableCell at the given row and col positions.
 *
 * @param table		The table to use
 * @param row		The index of the row (it starts with 0)
 * @param col		The index of the column (it starts with 0)
 * @return the TableCell at the given coordinates or NULL on error
 */
API TableCell *getTableCell(Table *table, int row, int col)
{
	if(table == NULL) {
		return NULL;
	}

	if(row >= table->rows) {
		return NULL;
	}

	if(col >= table->cols) {
		return NULL;
	}

	return &(table->table[row][col]);
}

/**
 * Adds new column(s) to the end of the table.
 *
 * If the table is empty (Table.table is NULL) a new row is created to add new columns.
 *
 * Because of performance reasons we recommend to create first all the columns
 * you need. Appending a new column after adding multiple rows will cause bad performance
 * as the column has to be added in every row.
 *
 * @param table			The table to extend with column(s)
 * @param colAmount		Amount of columns to add. Cannot be 0
 * @param cellTemplate	A TableCell which is used as a template for the newly added cells. Can be NULL.
 * @return the index of the first newly added column or a value lower 0 on error
 */
API int appendTableCol(Table *table, int colAmount, TableCell *cellTemplate)
{
	if(colAmount == 0) {
		return -1;
	}

	if(table->table == NULL) {
		appendTableRow(table, 1, cellTemplate);
	}

	int colCount = table->cols + colAmount;
	for(int row = 0; row < table->rows; row++) {

		// Create the space to store new column(s)
		TableCell *extendedCols = (TableCell *)realloc(table->table[row], sizeof(TableCell) * colCount);
		if(extendedCols == NULL) {
			exit(EXIT_FAILURE); // TODO: see appendTableRow
		}

		table->table[row] = extendedCols;

		// Fill rows with new TableCells for the new column
		for(int col = table->cols; col < colCount; col++) {
			if(cellTemplate == NULL) {
				table->table[row][col] = *(newTableCell(table));
			} else {
				table->table[row][col] = *(copyTableCell(table, cellTemplate));
			}
		}
	}

	int firstIndex = table->cols;
	table->cols = table->cols + colAmount;

	return firstIndex;
}

/**
 * Adds new row(s) to the end of the table.
 *
 * This command should be used after all the needed columns were added because of
 * better performance. @see appendTableCol
 *
 * @param table			The table to extend with row(s)
 * @param rowAmount		Amount of rows to add. Cannot be 0
 * @param cellTeplate	A TableCell which is used as a template for the newly added cells. Can be NULL.
 * @return the index of the first newly added row or a value lower 0 on error
 */
API int appendTableRow(Table *table, int rowAmount, TableCell *cellTemplate)
{
	if(rowAmount == 0) {
		return -1;
	}

	int rowCount = table->rows + rowAmount;

	// Create the space to store new row(s)
	if(table->table == NULL) {
		table->table = ALLOCATE_OBJECT(TableCell **);
	} else {
		TableCell **extendedTable = (TableCell **)realloc(table->table, sizeof(TableCell *) * rowCount);
		if(extendedTable == NULL) {
			exit(EXIT_FAILURE); // TODO: Create function/macro for the code above in memory_alloc.c/.h
		}

		table->table = extendedTable;
	}

	// Fill new rows with TableCells
	for(int row = table->rows; row < rowCount; row++) {
		table->table[row] = ALLOCATE_OBJECTS(TableCell, table->cols);

		for(int col = 0; col < table->cols; col++) {
			if(cellTemplate == NULL) {
				table->table[row][col] = *(newTableCell(table));
			} else {
				table->table[row][col] = *(copyTableCell(table, cellTemplate));
			}
		}
	}

	int firstIndex = table->rows;
	table->rows = table->rows + rowAmount;

	return firstIndex;
}

/**
 * Returns a copy of the given TableCell.
 *
 * To copy the TableCell.freeCell function and TableCell.tag,
 * Table.copyCall function is called.
 *
 * @param table		The table which contains the cells
 * @param original	The cell to copy
 * @return a copy of the given TableCell
 */
API TableCell *copyTableCell(Table *table, TableCell *original)
{
	TableCell *copy = newTableCell(table);
	copy->content = original->content != NULL ? strdup(original->content) : NULL;
	copy->format = original->format;

	if(table->copyCell) {
		table->copyCell(table, original, copy);
	}

	return copy;
}

/**
 * Replaces a cell by the given TableCell.
 *
 * @param table		The table which contains the cell
 * @param cell		The cell to use as replacement
 * @param row		The row index of the cell (it starts with 0)
 * @param col		The column index of the cell (it starts with 0)
 * @ret true if the replacement was successful, else false
 */
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
	freeCell(&table->table[row][col]);

	// set new cell
	table->table[row][col] = *cell;

	return true;
}

/**
 * Sets the content of a given cell. Always use this function as
 * it checks the length of the content and updates Table.longestContentLength.
 *
 * @param table		The table to which the cell belongs
 * @param cell		The cell to use
 * @param content	The new content. The content must be UTF-8 encoded
 */
API void setTableCellContent(Table *table, TableCell *cell, char *content)
{
	// free old content
	free(cell->content);

	if(content != NULL) {
		cell->content = strdup(content);

		int contentLength = g_utf8_strlen(content, -1);
		if(contentLength > table->longestContentLength) {
			table->longestContentLength = contentLength;
		}
	} else {
		cell->content = NULL;
	}
}

/**
 * Calls the table specific output generator function.
 *
 * @see Table.outputGenerator
 * @param table		The table to convert to a string
 * @return the string representing the table or NULL on error or missing generator function
 */
API char *getTableString(Table *table)
{
	if(table->outputGenerator != NULL) {
		return table->outputGenerator(table);
	} else {
		return NULL;
	}
}
