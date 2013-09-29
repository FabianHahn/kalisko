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
using namespace std;

#include "dll.h"
#define API
#include "Cell.h"

/**
 * Implementation of functions for class Cell:
 * constructor, destructor, getter for neighbours, getter & setter for borders & content
 */

Cell::Cell(Grid *parentGrid, int posX, int posY, int value) :
grid(parentGrid), x(posX), y(posY), content(value), topBorder(unknown), leftBorder(unknown)
{

}

Cell::~Cell()
{

}

Cell& Cell::getTopNeighbour()
{
	return grid->getCell(x - 1, y);
}

const Cell& Cell::getTopNeighbour() const
{
	return grid->getCell(x - 1, y);
}

Cell& Cell::getBottomNeighbour()
{
	return grid->getCell(x + 1, y);
}

const Cell& Cell::getBottomNeighbour() const
{
	return grid->getCell(x + 1, y);
}

Cell& Cell::getLeftNeighbour()
{
	return grid->getCell(x, y - 1);
}

const Cell& Cell::getLeftNeighbour() const
{
	return grid->getCell(x, y - 1);
}

Cell& Cell::getRightNeighbour()
{
	return grid->getCell(x, y + 1);
}

const Cell& Cell::getRightNeighbour() const
{
	return grid->getCell(x, y + 1);
}

Cell::State Cell::getTopBorder() const
{
	return topBorder;
}

Cell::State Cell::getBottomBorder() const
{
	return getBottomNeighbour().getTopBorder();
}

Cell::State Cell::getLeftBorder() const
{
	return leftBorder;
}

Cell::State Cell::getRightBorder() const
{
	return getRightNeighbour().getLeftBorder();
}

void Cell::setTopBorder(Cell::State state)
{
	topBorder = state;
}

void Cell::setBottomBorder(Cell::State state)
{
	getBottomNeighbour().setTopBorder(state);
}

void Cell::setLeftBorder(Cell::State state)
{
	leftBorder = state;
}

void Cell::setRightBorder(Cell::State state)
{
	getRightNeighbour().setLeftBorder(state);
}

int Cell::getContent() const
{
	return content;
}

void Cell::setContent(int c)
{
	content = c;
}

char Cell::getStateChar(State state, bool horizontal)
{
	switch(state) {
		case unknown:
			return ' ';
		case used:
			if(horizontal) {
				return '-';
			} else {
				return '|';
			}
		case unused:
			return 'x';
		default:
			return ' ';
	}
}
