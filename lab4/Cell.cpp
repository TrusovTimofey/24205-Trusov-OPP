#include "Cell.h"

Cell::Cell(BitArray* array, unsigned int index) : _array(array), _index(index){}

CellState Cell::state() const {
	return CellState((int)((*_array)[_index]));
}

void Cell::setState(CellState state) {
	(*_array)[_index] = (bool)state;
}
