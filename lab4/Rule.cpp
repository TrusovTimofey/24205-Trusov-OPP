#include "Rule.h"

using namespace std;

Rule::Rule()
{

}

Rule::Rule(const std::vector<unsigned char>& neighborsToBeBorn, const std::vector<unsigned char>& neighborsToSurvive)
{
	_isCreated = true;
	for (auto n : neighborsToBeBorn) if (!_neighborsToBeBorn.count(n)) _neighborsToBeBorn.insert(n);
	for (auto n : neighborsToSurvive) if (!_neighborsToSurvive.count(n)) _neighborsToSurvive.insert(n);
}

void Rule::create(const std::vector<unsigned char>& neighborsToBeBorn, const std::vector<unsigned char>& neighborsToSurvive)
{
	if (_isCreated) throw logic_error("Alredy created");
	_isCreated = true;
	_neighborsToBeBorn.clear();
	_neighborsToSurvive.clear();

	string error;

	for (auto n : neighborsToBeBorn)
	{
		try {
			if (_neighborsToBeBorn.count(n)) throw invalid_argument(to_string(n) + " is alredy in B rule");
			_neighborsToBeBorn.insert(n);
		}
		catch (const invalid_argument& e) {
			error += e.what() + "\n"s;
		}

	}

	for (auto n : neighborsToSurvive)
	{
		try {
			if (_neighborsToSurvive.count(n)) throw invalid_argument(to_string(n) + " is alredy in S rule");
			_neighborsToSurvive.insert(n);
		}
		catch (const invalid_argument& e) {
			error += e.what() + "\n"s;
		}

	}

	if (!error.empty()) throw invalid_argument(error);
}

bool Rule::mustToBeBorn(unsigned char neighborsCount) const
{
	if (!_isCreated) throw logic_error("Rule was not created");
	return _neighborsToBeBorn.count(neighborsCount);
}

bool Rule::mustToSurvive(unsigned char neighborsCount) const
{
	if (!_isCreated) throw logic_error("Rule was not created");
	return _neighborsToSurvive.count(neighborsCount);
}

string Rule::to_str() const {
	if (!_isCreated) throw logic_error("Rule was not created");
	string str;
	str.push_back('B');
	for (auto c : _neighborsToBeBorn) {
		str += to_string(c);
	}
	str.push_back('S');
	for (auto c : _neighborsToSurvive) {
		str += to_string(c);
	}
	return str;
}
