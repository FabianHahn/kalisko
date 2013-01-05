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


#ifndef TABLE_TABLE_H
#define TABLE_TABLE_H

// forward declarations
struct Table;
struct TableCell;

typedef char *(OutputGeneratorCallback)(struct Table *table);
typedef void (FreeTableCallback)(struct Table *table);
typedef void (FreeCellCallback)(struct TableCell *cell);
typedef void (NewCellCallback)(struct Table *table, struct TableCell *newCell);
typedef void (CopyCellCallback)(struct Table *table, struct TableCell *original, struct TableCell *copy);

/**
 * Represents a single cell of a table. It can hold a string as content.
 */
struct TableCell {
	/**
	 * Content of the cell. Can be null. Do not change it
	 * directly, use instead setTableCellContent function.
	 * The content should be encoded in UTF-8 (or ASCII).
	 *
	 * The content is only freed if freeContent is true.
	 *
	 * @see setTableCellContent
	 */
	char *content;

	/**
	 * Tells the Table module if it has to free the content.
	 *
	 * Default value is false.
	 */
	bool freeContent;

	/**
	 * Reference to additional data for the cell provided by
	 * a more specific Table implementation.
	 *
	 * This must be freed by the custom freeTable function.
	 */
	void *tag;

	/**
	 * A callback to free the content of @see tag.
	 *
	 * @see tag
	 */
	FreeCellCallback *freeCellCallback;
};

typedef struct TableCell TableCell;

/**
 * Represents a Table. It can be extended to generate different outputs and is useful
 * for different use cases. This is more or less a basic "class" for all tables containing
 * text.
 */
struct Table {
	/**
	 * Two dimensional array representing the cell. First index is the row and
	 * the second index is the column.
	 *
	 * Do not free it with the custom freeTable function.
	 */
	TableCell ***table;

	/**
	 * Column count.
	 */
	int cols;

	/**
	 * Row count.
	 */
	int rows;

	/**
	 * Count of allocated but not used rows.
	 */
	int freeRowsAmount;

	/**
	 * Count of allocated but not used columns.
	 */
	int freeColsAmount;

	/**
	 * Reference to additional data for the table provided by
	 * a more specific Table implementation.
	 *
	 * Must be freed by freeTable function.
	 */
	void *tag;

	/**
	 * A callback function to generate a string out of the Table.
	 *
	 * Can be null.
	 */
	OutputGeneratorCallback *outputGeneratorCallback;

	/**
	 * A callback to free the content of tag.
	 *
	 * @see tag
	 */
	FreeTableCallback *freeTableCallback;

	/**
	 * A callback to apply the specific stuff to a new TableCell.
	 */
	NewCellCallback *newCellCallback;

	/**
	 * A callback to apply a specific stuff to a copy of a TableCell.
	 */
	CopyCellCallback *copyCellCallback;
};

typedef struct Table Table;

#ifndef MODULE_TABLE_DEFAULT_ALLOC_ROWS
	/**
	 * Defines the default count of rows to allocate if you call newTable function.
	 *
	 * @see newTable
	 */
	#define MODULE_TABLE_DEFAULT_ALLOC_ROWS 10
#endif

#ifndef MODULE_TABLE_DEFAULT_ALLOC_COLS
	/**
	 * Defines the default count of columns to allocate if you call newTable function.
	 *
	 * @see newTable
	 */
	#define MODULE_TABLE_DEFAULT_ALLOC_COLS 3
#endif


/**
 * Returns a newly created Table.
 *
 * @return the new Table
 */
API Table *newTable();

/**
 * Returns a newly created Table with already allocated rows and columns.
 *
 * This functions just allocate space but not create the TableCells. So you still
 * have to use the append* functions for example.
 *
 * @param preAllocRows		Amount of rows to allocate
 * @param preAllocCols		Amount of columns to allocate
 * @return the new Table
 */
API Table *newTableFull(int preAllocRows, int preAllocCols);

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
API TableCell *newTableCell(Table *table);

/**
 * Frees the given table.
 *
 * It calls also the TableCell.freeCell and Table.freeTable functions if
 * they are set.
 *
 * @param table		The table to free
 */
API void freeTable(Table *table);

/**
 * Frees the given TableCell
 *
 * @param cell	The cell to free
 */
API void freeCell(TableCell *cell);

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
API int appendTableCol(Table *table, int colAmount, TableCell *cellTemplate);

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
API int appendTableRow(Table *table, int rowAmount, TableCell *cellTemplate);

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
API TableCell *copyTableCell(Table *table, TableCell *original);

/**
 * Replaces a cell by the given TableCell.
 *
 * @param table		The table which contains the cell
 * @param cell		The cell to use as replacement
 * @param row		The row index of the cell (it starts with 0)
 * @param col		The column index of the cell (it starts with 0)
 * @return true if the replacement was successful, else false
 */
API bool replaceTableCell(Table *table, TableCell *cell, int row, int col);

/**
 * Calls the table specific output generator function.
 *
 * @see Table.outputGenerator
 * @param table		The table to convert to a string
 * @return the string representing the table or NULL on error or missing generator function
 */
API char *getTableString(Table *table);

#endif
