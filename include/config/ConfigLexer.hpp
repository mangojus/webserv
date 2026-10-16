#pragma once

#include "config/Config.hpp"
#include "config/ConfigToken.hpp"
#include <string>
#include <fstream>

class ConfigLexer
{
	public:
		ConfigLexer() {}
		~ConfigLexer() {}

		std::vector<ConfigToken> tokenize(std::ifstream& file);

	private:
		ConfigLexer(const ConfigLexer& copy);
		ConfigLexer& operator=(const Config& other);

		ConfigToken createMultiCharToken(const std::string& line, size_t& index, const std::string& lineNumber);
};

