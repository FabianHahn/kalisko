/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
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

#include <iostream>

#include "dll.h"
#define API
#include "output.h"

std::ostream& operator<<(std::ostream& stream, const Grid& grid)
{
	int m = grid.getNumRows();
	int n = grid.getNumCols();

	for(int i = 0; i < m; i++) {
		for(int j = 0; j < n; j++) {
			const Cell& cell = grid.getCell(i, j);
			stream << "." << Cell::getStateChar(cell.getTopBorder(), true);
		}

		stream << "." << std::endl;

		for(int j = 0; j < n; j++) {
			const Cell& cell = grid.getCell(i, j);
			int content = cell.getContent();
			stream << Cell::getStateChar(cell.getLeftBorder(), false);

			if(content < 0) {
				stream << ' ';
			} else {
				stream << content;
			}
		}

		stream << Cell::getStateChar(grid.getCell(i, n).getLeftBorder(), false) << std::endl;
	}

	for(int j = 0; j < n; j++) {
		const Cell& cell = grid.getCell(m, j);
		stream << "." << Cell::getStateChar(cell.getTopBorder(), true);
	}

	stream << "." << std::endl;

	return stream;
}
