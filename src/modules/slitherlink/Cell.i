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


#ifndef SLITHERLINK_CELL_H
#define SLITHERLINK_CELL_H

#ifdef __cplusplus

#include <vector>
#include "Grid.h"

class Cell
{
public:
	typedef enum {
		unknown, used, unused
	} State;

	Cell(Grid *parentGrid, int posX, int posY, int value);
	virtual ~Cell();
	State getTopBorder() const;
	State getBottomBorder() const;
	State getLeftBorder() const;
	State getRightBorder() const;
	void setTopBorder(State state);
	void setBottomBorder(State state);
	void setLeftBorder(State state);
	void setRightBorder(State state);
	Cell& getTopNeighbour();
	const Cell& getTopNeighbour() const;
	Cell& getBottomNeighbour();
	const Cell& getBottomNeighbour() const;
	Cell& getLeftNeighbour();
	const Cell& getLeftNeighbour() const;
	Cell& getRightNeighbour();
	const Cell& getRightNeighbour() const;
	int getContent() const;
	void setContent(int c);

	static char getStateChar(State state, bool horizontal);

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
