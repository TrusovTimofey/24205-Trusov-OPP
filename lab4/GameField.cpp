#include "GameField.h"
#include "GameField.h"
#include <stdexcept>
#include <string>
#include "Globals.h"
#include <mpi.h>

using namespace std;

GameField::FieldCell::FieldCell(BitArray *array, unsigned int index) : Cell(array, index) {}

char GameField::FieldCell::neighborsCount() const { return _neighborsCount; }
void GameField::FieldCell::setNeighbors(char count)
{
	_neighborsCount = count;
}
void GameField::FieldCell::addNeighbor() { _neighborsCount++; }
void GameField::FieldCell::delNeighbor() { _neighborsCount--; }

GameField::GameField(int width, int height)
{
	_width = width;
	_height = height;

	int size = width * height;

	_fieldState = BitArray(size);
	_fieldState.reset();

	_upDeltaNeighbors = new char[width]();
	_downDeltaNeighbors = new char[width]();
	_upBuffer = new char[width];
	_downBuffer = new char[width];

	for (int i = 0; i < size; i++)
	{
		_field.push_back(FieldCell(&_fieldState, i));
	}
}
GameField::~GameField()
{
	delete[] _upDeltaNeighbors;
	delete[] _downDeltaNeighbors;
	delete[] _upBuffer;
	delete[] _downBuffer;
}

int GameField::index(int xPos, int yPos) const
{
	xPos = ((abs(xPos) / _width + 1) * _width + xPos) % _width;
	yPos = ((abs(yPos) / _height + 1) * _height + yPos) % _height;
	return xPos + yPos * _width;
}

GameField::FieldCell &GameField::cell(int xPos, int yPos)
{
	return _field[index(xPos, yPos)];
}

const GameField::FieldCell &GameField::cell(int xPos, int yPos) const
{
	return _field[index(xPos, yPos)];
}

vector<GameField::FieldCell *> GameField::getCellNeighbors(int xPos, int yPos)
{
	vector<FieldCell *> neighbors;

	if (yPos != 0)
	{
		neighbors.push_back(&cell(xPos - 1, yPos - 1));
		neighbors.push_back(&cell(xPos, yPos - 1));
		neighbors.push_back(&cell(xPos + 1, yPos - 1));
	}

	if (yPos != _height - 1)
	{
		neighbors.push_back(&cell(xPos - 1, yPos + 1));
		neighbors.push_back(&cell(xPos, yPos + 1));
		neighbors.push_back(&cell(xPos + 1, yPos + 1));
	}

	neighbors.push_back(&cell(xPos + 1, yPos));
	neighbors.push_back(&cell(xPos - 1, yPos));

	return neighbors;
}

int GameField::width() const
{
	return _width;
}

int GameField::height() const
{
	return _height;
}

bool GameField::cellIsAlive(int xPos, int yPos) const
{
	return cell(xPos, yPos).state() == CellState::Alive;
}

char GameField::cellNeighborsCount(int xPos, int yPos) const
{
	return cell(xPos, yPos).neighborsCount();
}

void GameField::bornCell(int xPos, int yPos)
{
	auto &c = cell(xPos, yPos);

	if (c.state() == CellState::Alive)
		throw logic_error("Cell is alredy alive");
	c.setState(CellState::Alive);

	if (yPos == 0)
	{
		_upDeltaNeighbors[index(xPos - 1, 0)]++;
		_upDeltaNeighbors[index(xPos, 0)]++;
		_upDeltaNeighbors[index(xPos + 1, 0)]++;
	}

	if (yPos == _height - 1)
	{
		_downDeltaNeighbors[index(xPos - 1, 0)]++;
		_downDeltaNeighbors[index(xPos, 0)]++;
		_downDeltaNeighbors[index(xPos + 1, 0)]++;
	}

	for (auto neighbor : getCellNeighbors(xPos, yPos))
	{
		neighbor->addNeighbor();
	}
}

void GameField::killCell(int xPos, int yPos)
{
	auto &c = cell(xPos, yPos);

	if (c.state() == CellState::Dead)
		throw logic_error("Cell is alredy dead");
	c.setState(CellState::Dead);

	if (yPos == 0)
	{
		_upDeltaNeighbors[index(xPos - 1, 0)]--;
		_upDeltaNeighbors[index(xPos, 0)]--;
		_upDeltaNeighbors[index(xPos + 1, 0)]--;
	}

	if (yPos == _height - 1)
	{
		_downDeltaNeighbors[index(xPos - 1, 0)]--;
		_downDeltaNeighbors[index(xPos, 0)]--;
		_downDeltaNeighbors[index(xPos + 1, 0)]--;
	}

	for (auto neighbor : getCellNeighbors(xPos, yPos))
	{
		neighbor->delNeighbor();
	}
}

void GameField::sendBorders()
{
	if (Globals::size == 1)
	{
		for (int i = 0; i < _width; i++)
		{
			auto &c1 = cell(i, _height - 1);
			c1.setNeighbors(c1.neighborsCount() + _upDeltaNeighbors[i]);
			_upDeltaNeighbors[i] = 0;

			auto &c2 = cell(i, 0);
			c2.setNeighbors(c2.neighborsCount() + _downDeltaNeighbors[i]);
			_downDeltaNeighbors[i] = 0;
		}
		return;
	}

	int rank = Globals::rank;
	int size = Globals::size;

	int upperRank = (rank - 1 + size) % size;
	int lowerRank = (rank + 1) % size;

	_requests = new MPI_Request[4];
	MPI_Request *requests = (MPI_Request *)_requests;
	int req_count = 0;

	MPI_Isend(_upDeltaNeighbors, _width, MPI_CHAR, upperRank, 0, MPI_COMM_WORLD, &requests[req_count++]);
	MPI_Irecv(_upBuffer, _width, MPI_CHAR, lowerRank, 0, MPI_COMM_WORLD, &requests[req_count++]);

	MPI_Isend(_downDeltaNeighbors, _width, MPI_CHAR, lowerRank, 0, MPI_COMM_WORLD, &requests[req_count++]);
	MPI_Irecv(_downBuffer, _width, MPI_CHAR, upperRank, 0, MPI_COMM_WORLD, &requests[req_count++]);
}

void GameField::applyBorders()
{
	if (Globals::size == 1)
		return;
	MPI_Waitall(4, (MPI_Request *)_requests, MPI_STATUSES_IGNORE);

	for (int i = 0; i < _width; i++)
	{
		auto &c = cell(i, _height - 1);
		c.setNeighbors(c.neighborsCount() + _upBuffer[i]);
		_upDeltaNeighbors[i] = 0;
	}

	for (int i = 0; i < _width; i++)
	{
		auto &c = cell(i, 0);
		c.setNeighbors(c.neighborsCount() + _downBuffer[i]);
		_downDeltaNeighbors[i] = 0;
	}
	delete[] (MPI_Request *)_requests;
}

BitArray GameField::getSnapShot() const
{
	return BitArray(_fieldState);
}

vector<vector<bool>> GameField::getCells() const
{
	vector<vector<bool>> values;
	values.resize(_width);

	for (int x = 0; x < _width; x++)
	{
		values[x].resize(_height);

		for (int y = 0; y < _height; y++)
		{
			values[x][y] = cell(x, y).state() == CellState::Alive;
		}
	}

	return values;
}

string GameField::to_str(char alive, char dead) const
{
	string str;
	for (int y = 0; y < _height; y++)
	{
		for (int x = 0; x < _width; x++)
		{
			str.push_back(cell(x, y).state() == CellState::Alive ? alive : dead);
		}
		str.push_back('\n');
	}
	return str;
}
