#include "utils/UniquePair.hpp"

/*
A = ("127.0.0.1", 8080)
B = ("127.0.0.1", 8080)
C = ("0.0.0.0", 8080)
!(a < b) && !(b < a)

a < b -> false
b < a -> false
a == b
*/

bool UniquePair::operator<(const UniquePair &other) const
{
	if (host < other.host)
		return true;
	else if (host > other.host)
		return false;
	return (port < other.port);
}