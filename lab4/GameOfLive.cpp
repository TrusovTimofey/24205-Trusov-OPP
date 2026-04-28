#include "GameOfLive.h"
#include <stdexcept>
#include <vector>
#include <list>
#include <mpi.h>
#include "Globals.h"

using namespace std;

GameOfLive::GameOfLive(Rule *rule, GameField *field) : _rule(rule), _field(field)
{
	_snapShots.push_back({_generation, field->getSnapShot()});
}

std::list<unsigned int> GameOfLive::repeatedSnapShots(BitArray &snapShot) const
{
	std::list<unsigned int> repeats;
	for (auto &gen : _snapShots)
	{

		if (gen.second == snapShot)
			repeats.push_back(gen.first);
	}
	return repeats;
}

bool GameOfLive::nextGeneration(int generations)
{
	if (generations <= 0)
		throw invalid_argument("generations must be greater then zero");

	for (int i = 0; i < generations; i++)
	{
		_generation++;
		vector<pair<bool, char>> cells;
		cells.resize(_field->width() * _field->height());
		for (int y = 0; y < _field->height(); y++)
		{
			for (int x = 0; x < _field->width(); x++)
			{
				cells[x + y * _field->width()] = pair<bool, char>(_field->cellIsAlive(x, y), _field->cellNeighborsCount(x, y));
			}
		}

		for (int y = 0; y < _field->height(); y++)
		{
			for (int x = 0; x < _field->width(); x++)
			{
				auto &cell = cells[x + y * _field->width()];
				if (cell.first)
				{
					if (!_rule->mustToSurvive(cell.second))
						_field->killCell(x, y);
				}
				else
				{
					if (_rule->mustToBeBorn(cell.second))
						_field->bornCell(x, y);
				}
			}
		}
		_field->applyChanges();
		
		auto snapShot = _field->getSnapShot();
		auto repeats = repeatedSnapShots(snapShot);
		std::vector<unsigned char> counter(_generation,0);
		
		for (auto repeat : repeats)
		{
			counter[repeat] = 1;
		}
		
		MPI_Allreduce(MPI_IN_PLACE, counter.data(), _generation, MPI_UNSIGNED_CHAR, MPI_SUM, MPI_COMM_WORLD);
		
		for (unsigned int i = 0; i < _generation; i++)
		{
			if (counter[i] >= Globals::size)
			return false;
		}
		_snapShots.push_back({_generation, snapShot});
		
	}
	return true;
}

unsigned int GameOfLive::generation()
{
	return _generation;
}

GameField *GameOfLive::field()
{
	return _field;
}
