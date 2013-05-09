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
	Grid test1(4,4); // simple 4 x 4 riddle-example
	test1.getCell(0,0).setContent(1);
	test1.getCell(0,1).setContent(-1);
	test1.getCell(0,2).setContent(-1);
	test1.getCell(0,3).setContent(0);
	test1.getCell(1,0).setContent(-1);
	test1.getCell(1,1).setContent(-1);
	test1.getCell(1,2).setContent(-1);
	test1.getCell(1,3).setContent(-1);
	test1.getCell(2,0).setContent(1);
	test1.getCell(2,1).setContent(-1);
	test1.getCell(2,2).setContent(2);
	test1.getCell(2,3).setContent(1);
	test1.getCell(3,0).setContent(-1);
	test1.getCell(3,1).setContent(2);
	test1.getCell(3,2).setContent(3);
	test1.getCell(3,3).setContent(-1);

	if (test1.checkContentToBorder()){ // should return negative, since riddle not solved yet
		cout << "Riddle solved correctly!" << endl;
	}else{
		cout << "Riddle not solved correctly!" << endl;
	}

	test1.getCell(0,0).setTopBorder(Cell::unused); // insert solution
	test1.getCell(0,0).setLeftBorder(Cell::unused);

	test1.getCell(0,1).setTopBorder(Cell::used);
	test1.getCell(0,1).setLeftBorder(Cell::used);

	test1.getCell(0,2).setTopBorder(Cell::unused);
	test1.getCell(0,2).setLeftBorder(Cell::used);

	test1.getCell(0,3).setTopBorder(Cell::unused);
	test1.getCell(0,3).setLeftBorder(Cell::unused);
	test1.getCell(0,3).setRightBorder(Cell::unused);

	test1.getCell(1,0).setTopBorder(Cell::unused);
	test1.getCell(1,0).setLeftBorder(Cell::unused);

	test1.getCell(1,1).setTopBorder(Cell::unused);
	test1.getCell(1,1).setLeftBorder(Cell::used);

	test1.getCell(1,2).setTopBorder(Cell::unused);
	test1.getCell(1,2).setLeftBorder(Cell::used);

	test1.getCell(1,3).setTopBorder(Cell::unused);
	test1.getCell(1,3).setLeftBorder(Cell::unused);
	test1.getCell(1,3).setRightBorder(Cell::unused);

	test1.getCell(2,0).setTopBorder(Cell::unused);
	test1.getCell(2,0).setLeftBorder(Cell::unused);

	test1.getCell(2,1).setTopBorder(Cell::unused);
	test1.getCell(2,1).setLeftBorder(Cell::used);

	test1.getCell(2,2).setTopBorder(Cell::used);
	test1.getCell(2,2).setLeftBorder(Cell::unused);

	test1.getCell(2,3).setTopBorder(Cell::unused);
	test1.getCell(2,3).setLeftBorder(Cell::used);
	test1.getCell(2,3).setRightBorder(Cell::unused);

	test1.getCell(3,0).setBottomBorder(Cell::unused);
	test1.getCell(3,0).setTopBorder(Cell::unused);
	test1.getCell(3,0).setLeftBorder(Cell::unused);

	test1.getCell(3,1).setBottomBorder(Cell::unused);
	test1.getCell(3,1).setTopBorder(Cell::used);
	test1.getCell(3,1).setLeftBorder(Cell::unused);

	test1.getCell(3,2).setBottomBorder(Cell::used);
	test1.getCell(3,2).setTopBorder(Cell::unused);
	test1.getCell(3,2).setLeftBorder(Cell::used);

	test1.getCell(3,3).setBottomBorder(Cell::unused);
	test1.getCell(3,3).setTopBorder(Cell::unused);
	test1.getCell(3,3).setLeftBorder(Cell::used);
	test1.getCell(3,3).setRightBorder(Cell::unused);

	if (test1.checkContentToBorder()){ // should return positive, since solution was entered
		cout << "Riddle solved correctly!" << endl;
	}else{
		cout << "Riddle not solved correctly!" << endl;
	}

}


/**
 * Implementation of functions for class Grid:
 * constructor, destructor, getter for rows & columns, checkContentToBorder
 */

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

bool Grid::checkContentToBorder(){  // compares content-value to the number of occupied borders
	bool check = true;
	for (int i=0; i<m; i++){
		for (int j=0; j<n; j++){
			int count = 0;
			if (getCell(i,j).getBottomBorder()==Cell::used){
				count++;
			}
			if (getCell(i,j).getTopBorder()==Cell::used){
				count++;
			}
			if (getCell(i,j).getLeftBorder()==Cell::used){
				count++;
			}
			if (getCell(i,j).getRightBorder()==Cell::used){
				count++;
			}
			int content = getCell(i,j).getContent();
			if (content>-1 && content != count){
				check = false;
				cout << "Number of borders in [" << i << "," << j << "] do not match the content." << endl;
			}
		}
	}
	return check;
}


/**
 * Implementation of functions for class Cell:
 * constructor, destructor, getter for neighbours, getter & setter for borders & content
 */

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

Cell::State Cell::getTopBorder(){
	return topBorder;
}

Cell::State Cell::getBottomBorder(){
	return getBottomNeighbour().getTopBorder();
}

Cell::State Cell::getLeftBorder(){
	return leftBorder;
}

Cell::State Cell::getRightBorder(){
	return getRightNeighbour().getLeftBorder();
}

void Cell::setTopBorder(Cell::State state){
	topBorder = state;
}

void Cell::setBottomBorder(Cell::State state){
	getBottomNeighbour().setTopBorder(state);
}

void Cell::setLeftBorder(Cell::State state){
	leftBorder = state;
}

void Cell::setRightBorder(Cell::State state){
	getRightNeighbour().setLeftBorder(state);
}


int Cell::getContent(){
	return content;
}

void Cell::setContent(int c){
	content = c;
}
