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
#include "generate.h"

API void generateSlitherlink()
{

}

Grid::Grid(int rows, int cols) : m(rows), n(cols), cells((m+1)*(n+1))
{
	for (int i=0; i<m+1; i++){
		for (int j=0; j<n+1; j++){
			cells[i*(n+1)+j] = new Cell(this,i,j,-1);
		}
	}
}

Grid::~Grid()
{
	for (int i=0; i<(m+1)*(n+1); i++){
		delete cells[i];
	}
}

int Grid::getNumRows(){
	return m;
}

int Grid::getNumCols(){
	return n;
}

Cell& Grid::getCell(int x, int y){
	return *cells[x*(n+1)+y];
}

Cell::Cell(Grid *parentGrid, int posX, int posY, int value) : grid(parentGrid), x(posX), y(posY), content(value), topBorder(unknown), leftBorder(unknown)
{

}

Cell::~Cell(){

}

Cell& Cell::getTopNeighbour(){
	return grid->getCell(x-1,y);
}

Cell& Cell::getBottomNeighbour(){
	return grid->getCell(x+1,y);
}

Cell& Cell::getLeftNeighbour(){
	return grid->getCell(x,y-1);
}

Cell& Cell::getRightNeighbour(){
	return grid->getCell(x,y+1);
}

Cell::State& Cell::getTopBorder(){
	return topBorder;
}

Cell::State& Cell::getBottomBorder(){
	return getBottomNeighbour().topBorder;
}

Cell::State& Cell::getLeftBorder(){
	return leftBorder;
}

Cell::State& Cell::getRightBorder(){
	return getRightNeighbour().leftBorder;
}

int Cell::getContent(){
	return content;
}

void Cell::setContent(int c){
	content = c;
}
