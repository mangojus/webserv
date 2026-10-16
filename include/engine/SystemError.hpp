
#pragma once
#include <stdexcept>
#include <string>
#include <cerrno>
#include <cstring>

class SystemError : public std::runtime_error
{
	public:
		explicit SystemError(const std::string& where)
			: std::runtime_error(where + ": " + std::string(std::strerror(errno)))
			{}
};