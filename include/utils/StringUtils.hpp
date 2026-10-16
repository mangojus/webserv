#pragma once
#include <string>

namespace utils
{
    std::string trim(const std::string& str);
	std::string smartTrim(const std::string& str, const std::string& set);
	bool isNumber(const std::string& str);
    int countWords(const std::string& str);
    std::string toLower(const std::string& str);
    std::string intToStr(int n);
}
