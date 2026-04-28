#pragma once

#include "BitArray.h"

enum class CellState : unsigned char {
	Dead = 0,
	Alive = 1
};

class Cell {
protected:
	BitArray* _array;
	unsigned int _index;
public:
	Cell(BitArray* array, unsigned int index);
	CellState state() const;
	void setState(CellState state);
};
