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
#include "Cell.h"
#include "Grid.h"
#include "generate.h"
#include "output.h"

API void generateSlitherlink()
{
	Grid test1(4, 4); // simple 4 x 4 riddle-example
	test1.getCell(0, 0).setContent(1);
	test1.getCell(0, 1).setContent(-1);
	test1.getCell(0, 2).setContent(-1);
	test1.getCell(0, 3).setContent(0);
	test1.getCell(1, 0).setContent(-1);
	test1.getCell(1, 1).setContent(-1);
	test1.getCell(1, 2).setContent(-1);
	test1.getCell(1, 3).setContent(-1);
	test1.getCell(2, 0).setContent(1);
	test1.getCell(2, 1).setContent(-1);
	test1.getCell(2, 2).setContent(2);
	test1.getCell(2, 3).setContent(1);
	test1.getCell(3, 0).setContent(-1);
	test1.getCell(3, 1).setContent(2);
	test1.getCell(3, 2).setContent(3);
	test1.getCell(3, 3).setContent(-1);

	if(test1.checkContentToBorder()) { // should return negative, since riddle not solved yet
		std::cout << "Riddle solved correctly!" << std::endl;
	} else {
		std::cout << "Riddle not solved correctly!" << std::endl;
	}

	test1.getCell(0, 0).setTopBorder(Cell::unused); // insert solution
	test1.getCell(0, 0).setLeftBorder(Cell::unused);

	test1.getCell(0, 1).setTopBorder(Cell::used);
	test1.getCell(0, 1).setLeftBorder(Cell::used);

	test1.getCell(0, 2).setTopBorder(Cell::unused);
	test1.getCell(0, 2).setLeftBorder(Cell::used);

	test1.getCell(0, 3).setTopBorder(Cell::unused);
	test1.getCell(0, 3).setLeftBorder(Cell::unused);
	test1.getCell(0, 3).setRightBorder(Cell::unused);

	test1.getCell(1, 0).setTopBorder(Cell::unused);
	test1.getCell(1, 0).setLeftBorder(Cell::unused);

	test1.getCell(1, 1).setTopBorder(Cell::unused);
	test1.getCell(1, 1).setLeftBorder(Cell::used);

	test1.getCell(1, 2).setTopBorder(Cell::unused);
	test1.getCell(1, 2).setLeftBorder(Cell::used);

	test1.getCell(1, 3).setTopBorder(Cell::unused);
	test1.getCell(1, 3).setLeftBorder(Cell::unused);
	test1.getCell(1, 3).setRightBorder(Cell::unused);

	test1.getCell(2, 0).setTopBorder(Cell::unused);
	test1.getCell(2, 0).setLeftBorder(Cell::unused);

	test1.getCell(2, 1).setTopBorder(Cell::unused);
	test1.getCell(2, 1).setLeftBorder(Cell::used);

	test1.getCell(2, 2).setTopBorder(Cell::used);
	test1.getCell(2, 2).setLeftBorder(Cell::unused);

	test1.getCell(2, 3).setTopBorder(Cell::unused);
	test1.getCell(2, 3).setLeftBorder(Cell::used);
	test1.getCell(2, 3).setRightBorder(Cell::unused);

	test1.getCell(3, 0).setBottomBorder(Cell::unused);
	test1.getCell(3, 0).setTopBorder(Cell::unused);
	test1.getCell(3, 0).setLeftBorder(Cell::unused);

	test1.getCell(3, 1).setBottomBorder(Cell::unused);
	test1.getCell(3, 1).setTopBorder(Cell::used);
	test1.getCell(3, 1).setLeftBorder(Cell::unused);

	test1.getCell(3, 2).setBottomBorder(Cell::used);
	test1.getCell(3, 2).setTopBorder(Cell::unused);
	test1.getCell(3, 2).setLeftBorder(Cell::used);

	test1.getCell(3, 3).setBottomBorder(Cell::unused);
	test1.getCell(3, 3).setTopBorder(Cell::unused);
	test1.getCell(3, 3).setLeftBorder(Cell::used);
	test1.getCell(3, 3).setRightBorder(Cell::unused);

	if(test1.checkContentToBorder()) { // should return positive, since solution was entered
		std::cout << "Riddle solved correctly!" << std::endl;
	} else {
		std::cout << "Riddle not solved correctly!" << std::endl;
	}

	std::cout << test1 << std::endl;
}
