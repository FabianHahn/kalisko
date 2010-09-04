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
#include "string.h"
#include "util.h"
#include "modules/table/table.h"
#include "modules/plaintext_table/plaintext_table.h"

#include "api.h"

MODULE_NAME("test_table");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the table module");
MODULE_VERSION(0, 1, 4);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("table", 0, 1, 5));

static char *generatorFunc(Table *table);

TEST_CASE(basic_table_functions);
TEST_CASE(cell_template);
TEST_CASE(replace_table_cell);
TEST_CASE(generator);
TEST_CASE(pre_alloc);
TEST_CASE(no_pre_alloc);

TEST_SUITE_BEGIN(table)
	TEST_CASE_ADD(basic_table_functions);
	TEST_CASE_ADD(cell_template);
	TEST_CASE_ADD(replace_table_cell);
	TEST_CASE_ADD(generator);
	TEST_CASE_ADD(pre_alloc);
	TEST_CASE_ADD(no_pre_alloc);
TEST_SUITE_END

/**
 * Prints just every cell out in a comma seperated string
 *
 * @param table
 * @return
 */
static char *generatorFunc(Table *table)
{
	GString *str = g_string_new("");
	for(int row = 0; row < table->rows; row++) {
		for(int col = 0; col < table->cols; col++) {
			g_string_append_printf(str, "%s,", table->table[row][col]->content);
		}
	}

	char *out = str->str;
	g_string_free(str, false);

	return out;
}

TEST_CASE(basic_table_functions)
{
	Table *table = $(Table *, table, newTable)();
	TEST_ASSERT(table != NULL);

	int firstColIndex = $(int, table, appendTableCol)(table, 3, NULL);
	TEST_ASSERT(table->cols == 3);
	TEST_ASSERT(table->rows == 1);
	TEST_ASSERT(firstColIndex == 0);

	int firstRowIndex = $(int, table, appendTableRow)(table, 1, NULL);
	TEST_ASSERT(table->rows == 2);
	TEST_ASSERT(firstRowIndex == 1);

	TableCell *currentCell = table->table[0][0];
	TEST_ASSERT(currentCell != NULL);

	$(void, table, freeTable)(table);

	TEST_PASS;
}

TEST_CASE(cell_template)
{
	// create table
	Table *table = $(Table *, table, newTable)();
	TEST_ASSERT(table != NULL);

	// create template
	TableCell *tpl = $(TableCell *, table, newTableCell)(table);
	tpl->content = "foo";

	// create cols and rows
	$(int, table, appendTableCol)(table, 5, tpl);
	$(int, table, appendTableRow)(table, 5, tpl);

	// check the new cols and rows
	TEST_ASSERT(table->rows == 6);
	TEST_ASSERT(table->cols == 5);

	for(int row = 0; row < table->rows; row++) {
		for(int col = 0; col < table->cols; col++) {
			TEST_ASSERT(strcmp(table->table[row][col]->content, "foo") == 0);
		}
	}

	$(void, table, freeTable)(table);

	TEST_PASS;
}

TEST_CASE(replace_table_cell)
{
	// create table
	Table *table = $(Table *, table, newTable)();
	TEST_ASSERT(table != NULL);

	// create template
	TableCell *tpl = $(TableCell *, table, newTableCell)(table);
	tpl->content = "foo";

	// create cols and rows
	$(int, table, appendTableCol)(table, 5, tpl);
	$(int, table, appendTableRow)(table, 5, tpl);

	// check the new cols and rows
	TEST_ASSERT(table->rows == 6);
	TEST_ASSERT(table->cols == 5);

	// create replacement
	TableCell *replace = $(TableCell *, table, newTableCell)(table);
	TEST_ASSERT(replace != NULL);
	replace->content = "bar";

	// do the replacement
	TEST_ASSERT($(bool, table, replaceTableCell)(table, replace, 1, 1));

	// check the replacement
	TEST_ASSERT(strcmp(table->table[1][1]->content, "bar") == 0);
	TEST_ASSERT(strcmp(table->table[1][0]->content, "foo") == 0);

	$(void, table, freeTable)(table);

	TEST_PASS;
}

TEST_CASE(generator)
{
	// create table
	Table *table = $(Table *, table, newTable)();
	TEST_ASSERT(table != NULL);

	table->outputGenerator = &generatorFunc;

	// create template
	TableCell *tpl = $(TableCell *, table, newTableCell)(table);
	tpl->content = "foo";

	// create cols and rows
	$(int, table, appendTableCol)(table, 2, tpl);
	$(int, table, appendTableRow)(table, 1, tpl);

	// check the new cols and rows
	TEST_ASSERT(table->rows == 2);
	TEST_ASSERT(table->cols == 2);

	// generate string
	char *str = $(char *, table, getTableString)(table);

	// there must be four times "foo," as we have two rows and two cols (2 * 2 = 4)
	TEST_ASSERT(strcmp(str, "foo,foo,foo,foo,") == 0);

	free(str);
	$(void, table, freeTable)(table);

	TEST_PASS;
}

TEST_CASE(pre_alloc)
{
	Table *table = $(Table *, table, newTable)();
	TEST_ASSERT(table != NULL);
	TEST_ASSERT(table->freeColsAmount == MODULE_TABLE_DEFAULT_ALLOC_COLS);
	TEST_ASSERT(table->freeRowsAmount == MODULE_TABLE_DEFAULT_ALLOC_ROWS);
	TEST_ASSERT(table->rows == 0);
	TEST_ASSERT(table->cols == 0);

	TEST_ASSERT($(int, table, appendTableCol)(table, 2, NULL) == 0);
	TEST_ASSERT(table->freeColsAmount == (MODULE_TABLE_DEFAULT_ALLOC_COLS - 2));

	// Do not forget that after adding a column to a empty table there will be already a row
	TEST_ASSERT($(int, table, appendTableRow)(table, 1, NULL) == 1);
	TEST_ASSERT(table->freeRowsAmount == (MODULE_TABLE_DEFAULT_ALLOC_ROWS - 2));
	TEST_ASSERT(table->rows == 2);
	TEST_ASSERT(table->cols == 2);

	$(void, table, freeTable)(table);

	TEST_PASS;
}

TEST_CASE(no_pre_alloc)
{
	Table *table = $(Table *, table, newTableFull)(0, 0);
	TEST_ASSERT(table != NULL);
	TEST_ASSERT(table->table == NULL);
	TEST_ASSERT(table->freeColsAmount == 0);
	TEST_ASSERT(table->freeRowsAmount == 0);
	TEST_ASSERT(table->rows == 0);
	TEST_ASSERT(table->cols == 0);

	TEST_ASSERT($(int, table, appendTableCol)(table, 5, NULL) == 0);
	TEST_ASSERT($(int, table, appendTableRow)(table, 5, NULL) == 1);

	$(void, table, freeTable)(table);

	TEST_PASS;
}
