#include "config/ConfigLexer.hpp"

#include "config/ConfigToken.hpp"
#include "utils/StringUtils.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>

ConfigToken ConfigLexer::createMultiCharToken(const std::string& line, size_t& index, const std::string& lineNumber)
{
	size_t start = index;
	while (index < line.length()
			&& !isspace(line[index])
			&& line[index] != '{'
			&& line[index] != '}'
			&& line[index] != ';'
			&& line[index] != '#')
	{
		++index;
	}
	std::string tokenValue = line.substr(start, index - start);
	return ConfigToken(tokenValue, ID, lineNumber); 
}

std::vector<ConfigToken> ConfigLexer::tokenize(std::ifstream& file)
{
	std::string line;
	std::vector<ConfigToken> tokens;

	std::string lineStr;
	int lineNumber = 1;
	while (std::getline(file, line)) {
		size_t i = 0;
		while (i < line.length()) {
			if (isspace(line[i])) {
				++i;
				continue;
			}
			lineStr = utils::intToStr(lineNumber);
			switch (line[i]) {
				case '{':
					tokens.push_back(ConfigToken("{", LBRACE, lineStr));
					++i;
					break;
				case '}':
					tokens.push_back(ConfigToken("}", RBRACE, lineStr));
					++i;
					break;
				case ';':
					tokens.push_back(ConfigToken(";", SEMICOLON, lineStr));
					++i;
					break;
				case '#':
					i = line.length();
					break;
				default:
					tokens.push_back(createMultiCharToken(line, i, lineStr));
					break;
			}
		}
		++lineNumber;

	}
	tokens.push_back(ConfigToken("", END, lineStr));
	return tokens;
}
