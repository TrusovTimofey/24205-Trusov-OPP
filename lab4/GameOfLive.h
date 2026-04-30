#pragma once

#include "GameField.h"
#include "Rule.h"
#include <string>
#include <list>
#include <vector>


class GameOfLive
{
private:
	Rule* _rule;
	GameField* _field;

	std::vector<std::pair<bool, char>> _tempCells;
	std::vector<unsigned char> _genCounter;

	unsigned int _generation = 0;

	std::list<std::pair<unsigned int, BitArray>> _snapShots;

	std::list<unsigned int> repeatedSnapShots(BitArray& snapShot) const;

public:
	GameOfLive(Rule* rule, GameField* field);

	bool nextGeneration(int generations = 1);

	unsigned int generation();

	GameField* field();
};
