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


#ifndef PLAINTEXT_TABLE_PLAINTEXT_TABLE_H
#define PLAINTEXT_TABLE_PLAINTEXT_TABLE_H

#include "modules/table/table.h"

typedef struct PlaintextTableCellTag PlaintextTableCellTag;

/**
 * Defines the alignment of the content in a column.
 */
typedef enum {
	PLAINTEXT_TABLE_ALIGN_CENTER,
	PLAINTEXT_TABLE_ALIGN_RIGHT,
	PLAINTEXT_TABLE_ALIGN_LEFT
} PlaintextTableAlignment;

/**
 * The Plaintext Table module specific TableCell tag struct
 */
struct PlaintextTableCellTag {
	/**
	 * The alignment of the content of the cell
	 */
	PlaintextTableAlignment alignment;
};


/**
 * Creates a new Table which is prepared to create
 * output for the CLI or a plaintext file.
 *
 * @return the specialised Table
 */
API Table *newPlaintextTable();

/**
 * Creates a new Table which is prepared to create
 * output for the CLI or plaintext files.
 *
 * @see newTableFull
 * @param preAllocRows		Amount of rows to allocate
 * @param preAllocCols		Amount of columns to allocate
 */
API Table *newPlaintextTableFull(int preAllocRows, int preAllocCols);

#endif
