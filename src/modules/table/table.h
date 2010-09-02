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

typedef struct Table Table;
typedef struct TableCell TableCell;

typedef char *(OutputGeneratorCallback)(Table *table);
typedef void (FreeTableCallback)(Table *table);
typedef void (FreeCellCallback)(TableCell *cell);
typedef void (NewCellCallback)(Table *table, TableCell *newCell);
typedef void (CopyCellCallback)(Table *table, TableCell *original, TableCell *copy);

/**
 * Reprecents a single cell of a table. It can hold a string as content.
 */
struct TableCell {
	/**
	 * Content of the cell. Can be null. Do not change it
	 * directly, use instead setTableCellContent function.
	 *
	 * Do not free it with the custom freeTable function.
	 *
	 * @see setTableCellContent
	 */
	char *content;

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
	FreeCellCallback *freeCell;
};

/**
 * Represents a Table. It can be extended to generate different outputs and is useful
 * for different use cases. This is more or less a basic "class" for all tables containing
 * text.
 */
struct Table {
	/**
	 * Two dimensional array representing the cell. First index is the row and
	 * the second index is the col.
	 *
	 * The last col and last row is always NULL.
	 *
	 * Do not free it with the custom freeTable function.
	 */
	TableCell **table;

	/**
	 * Cache for the longest length of a cell content.
	 *
	 * Everytime a change to a content is made or new content is added this value may
	 * change.
	 */
	int longestContentLength;

	/**
	 * Cache for the amount of columns.
	 */
	int cols;

	/**
	 * Cache for the amount of rows.
	 */
	int rows;

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
	OutputGeneratorCallback *outputGenerator;

	/**
	 * A callback to free the content of @see tag.
	 *
	 * @see tag
	 */
	FreeTableCallback *freeTable;

	/**
	 * A callback to apply a specific TableCell.tag and TableCell.freeCell to a new TableCell
	 */
	NewCellCallback *newCell;

	/**
	 * A callback to apply a specific TableCell.tag and TableCell.freeCell to a copy of a TableCell
	 */
	CopyCellCallback *copyCell;
};

API Table *newTable();
API TableCell *newTableCell(Table *table);
API void freeTable(Table *table);
API void freeCell(TableCell *cell);
API int appendTableCol(Table *table, int colAmount, TableCell *cellTemplate);
API int appendTableRow(Table *table, int rowAmount, TableCell *cellTemplate);
API TableCell *getTableCell(Table *table, int row, int col);
API TableCell *copyTableCell(Table *table, TableCell *original);
API bool replaceTableCell(Table *table, TableCell *cell, int row, int col);
API void setTableCellContent(Table *table, TableCell *cell, char *content);
API char *getTableString(Table *table);

#ifdef DLL_API_IMPORT

#define setTableColRowContent(TABLE, CONTENT, ROW, COL) \
	$(void, table, setTableCellContent)(TABLE, &(TABLE->table[ROW][COL]), CONTENT)

#endif

#endif
