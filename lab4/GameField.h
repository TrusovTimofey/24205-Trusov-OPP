#pragma once

#include "Cell.h"
#include "BitArray.h"

#include <string>
#include <vector>

class GameField
{
private:
	int _width, _height;

	BitArray _fieldState;

	class FieldCell : public Cell {
	private:
		char _neighborsCount = 0;
	public:
	    FieldCell(BitArray* array, unsigned int index);
		char neighborsCount() const;
		void setNeighbors(char count);
		void addNeighbor();
		void delNeighbor();
	};

	std::vector<FieldCell> _field;

	char* _upDeltaNeighbors;
	char* _downDeltaNeighbors;
	char* _upBuffer;
	char* _downBuffer;

	int index(int xPos, int yPos) const;
	const FieldCell& cell(int xPos, int yPos) const;
	FieldCell& cell(int xPos, int yPos);
	std::vector<FieldCell*> getCellNeighbors(int xPos, int yPos);

public:
	GameField(int width, int height);
	~GameField();

	int width() const;
	int height() const;

	bool cellIsAlive(int xPos, int yPos) const;
	char cellNeighborsCount(int xPos, int yPos) const;

	void bornCell(int xPos, int yPos);
	void killCell(int xPos, int yPos);

	void applyChanges();

	BitArray getSnapShot() const;

	std::vector<std::vector<bool>> getCells() const;
	std::string to_str(char alive = '#', char dead = ' ') const;
};
