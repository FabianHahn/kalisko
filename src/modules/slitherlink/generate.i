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


#ifndef SLITHERLINK_GENERATE_H
#define SLITHERLINK_GENERATE_H

#ifdef __cplusplus
extern "C" {
#endif

API void generateSlitherlink();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <vector>
class Cell; // forward declaration

class Grid
{
public:
	Grid();
	virtual ~Grid();
	int getNumRows();
	int getNumCols();
	Cell& getCell(int x,int y);

private:
	int m;
	int n;
	vector<Cell *> grid;
};

class Cell
{
public:
	typedef enum {
		unknown, used, unused
	} State;

	Cell();
	virtual ~Cell();
	State& getTopBorder();
	State& getBottomBorder();
	State& getLeftBorder();
	State& getRightBorder();
	Cell& getTopNeighbour();
	Cell& getBottomNeighbour();
	Cell& getLeftNeighbour();
	Cell& getRightNeighbour();
	int getContent();
	void setContent(int c);

private:
	Grid *grid;
	int x;
	int y;
	int content;
	State topBorder;
	State leftBorder;
};

#endif

#endif
