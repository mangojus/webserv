#pragma once

#include <string>

struct UniquePair
{
	std::string host; //ip
	int port;

	UniquePair() : host(""), port(-1) {};
	UniquePair(const std::string &h, int p) : host(h), port(p) {};

	bool operator<(const UniquePair &other) const;
};
