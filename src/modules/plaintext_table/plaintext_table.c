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
#include <math.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "modules/table/table.h"
#include "memory_alloc.h"

#define API
#include "plaintext_table.h"


MODULE_NAME("plaintext_table");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A plaintext table for output in CLI or text files");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("table", 0, 1, 5));

static char *getPlaintextTableString(Table *table);
static void freePlaintextTableCell(TableCell *cell);
static void newPlaintextTableCellCallback(Table *table, TableCell *newCell);
static void copyPlaintextTableCellCallback(Table *table, TableCell *original, TableCell *copy);

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

API Table *newPlaintextTable()
{
	return newPlaintextTableFull(MODULE_TABLE_DEFAULT_ALLOC_ROWS, MODULE_TABLE_DEFAULT_ALLOC_COLS);
}

API Table *newPlaintextTableFull(int preAllocRows, int preAllocCols)
{
	Table *table = $(Table *, table, newTableFull)(preAllocRows, preAllocCols);

	table->newCellCallback = &newPlaintextTableCellCallback;
	table->outputGeneratorCallback = &getPlaintextTableString;
	table->copyCellCallback = &copyPlaintextTableCellCallback;

	return table;
}

/**
 * Creates the output for the table.
 *
 * @param table		The table to create output from
 * @return a string which must be freed containing the output
 */
static char *getPlaintextTableString(Table *table)
{
	GString *out = g_string_new("");

	// get max lengths
	int *maxLengths = ALLOCATE_OBJECTS(int, table->cols);
	memset(maxLengths, 0, sizeof(int) * table->cols);

	for(int row = 0; row < table->rows; row++) {
		for(int col = 0; col < table->cols; col++) {
			char *txt = table->table[row][col]->content;

			if(txt != NULL) {
				int length = g_utf8_strlen(txt, -1);
				if(length > maxLengths[col]) {
					maxLengths[col] = length;
				}
			}
		}
	}

	// write output
	for(int row = 0; row < table->rows; row++) {
		for(int col = 0; col < table->cols; col++) {
			TableCell *cell = table->table[row][col];
			char *txt = cell->content;

			if(txt != NULL) {
				int txtLength = g_utf8_strlen(txt, -1);
				int diff = maxLengths[col] - txtLength;

				if(diff != 0) {
					int spacesLeft = 0;
					int spacesRight = 0;

					switch(((PlaintextTableCellTag *)cell->tag)->alignment) {
						case PLAINTEXT_TABLE_ALIGN_CENTER:
							spacesLeft = (int)ceil(diff / 2);
							spacesRight = diff - spacesLeft;
							break;
						case PLAINTEXT_TABLE_ALIGN_LEFT:
							spacesRight = diff;
							break;
						case PLAINTEXT_TABLE_ALIGN_RIGHT:
							spacesLeft = diff;
							break;
					}

					char *spacesLeftStr = g_strnfill(spacesLeft, ' ');
					char *spacesRightStr = g_strnfill(spacesRight, ' ');

					g_string_append_printf(out, "%s%s%s", spacesLeftStr, txt, spacesRightStr);

					free(spacesLeftStr);
					free(spacesRightStr);
				} else {
					g_string_append(out, txt);
				}
			} else {
				// no content
				char *writeTxt = g_strnfill(maxLengths[col], ' ');
				g_string_append(out, writeTxt);

				free(writeTxt);
			}

			g_string_append(out, "    ");
		}

		g_string_append(out, "\n");
	}

	char *str = out->str;
	g_string_free(out, false);
	return str;
}

/**
 * Frees the specific stuff in a TableCell
 *
 * @param cell	The cell being freed
 */
static void freePlaintextTableCell(TableCell *cell)
{
	if(cell->tag != NULL) {
		free(cell->tag);
	}
}

/**
 * Adds the specific stuff to a new TableCell
 *
 * @param table		The table to which the TableCell is added
 * @param newCell	The new TableCell itself.
 */
static void newPlaintextTableCellCallback(Table *table, TableCell *newCell)
{
	newCell->freeCellCallback = &freePlaintextTableCell;

	PlaintextTableCellTag *tag = ALLOCATE_OBJECT(PlaintextTableCellTag);
	tag->alignment = PLAINTEXT_TABLE_ALIGN_LEFT;

	newCell->tag = tag;
}

/**
 * Copies the specific stuff to a new TableCell
 *
 * @param table		The table to which the TableCells belong
 * @param original	The original TableCell from which we get our data to copy
 * @param copy		The TableCell where we put the copied data into
 */
static void copyPlaintextTableCellCallback(Table *table, TableCell *original, TableCell *copy)
{
	PlaintextTableCellTag *tag = ALLOCATE_OBJECT(PlaintextTableCellTag);
	tag->alignment = ((PlaintextTableCellTag *)original->tag)->alignment;

	copy->tag = tag;
}
