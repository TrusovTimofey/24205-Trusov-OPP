#pragma once

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

class Rule
{
private:
	bool _isCreated = false;
	std::set<unsigned char> _neighborsToBeBorn;
	std::set<unsigned char> _neighborsToSurvive;
public:
	Rule();
	Rule(const std::vector<unsigned char>& neighborsToBeBorn, const std::vector<unsigned char>& neighborsToSurvive);
	void create(const std::vector<unsigned char>& neighborsToBeBorn, const std::vector<unsigned char>& neighborsToSurvive);

	bool mustToBeBorn(unsigned char neighborsCount) const;
	bool mustToSurvive(unsigned char neighborsCount) const;

	std::string to_str() const;
};
