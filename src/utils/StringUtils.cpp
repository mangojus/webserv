#include "utils/StringUtils.hpp"
#include <sstream>
#include <cctype>

namespace utils
{
    std::string trim(const std::string& str)
    {
        size_t start = 0;
        size_t end = str.length();

        while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
            ++start;

        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
            --end;

        return str.substr(start, end - start);
    }

	std::string smartTrim(const std::string& str, const std::string& set)
	{
		size_t start = str.find_first_not_of(set);
		if (start == std::string::npos)
			return "";
		size_t end = str.find_last_not_of(set);
		return str.substr(start, end - start + 1);
	}

    int countWords(const std::string& str)
    {
        std::istringstream iss(str);
        std::string word;
        int count = 0;

        while (iss >> word)
            ++count;

        return count;
    }

    std::string toLower(const std::string& str)
    {
        std::string result = str;

        for (size_t i = 0; i < result.length(); ++i)
        {
            result[i] = static_cast<char>(std::tolower(result[i]));
        }

        return result;
    }

	bool isNumber(const std::string& str)
	{
		if (str.empty()) {
			return false;
		}
		for (size_t i = 0; i < str.size(); ++i) {
			if (!isdigit(str[i])) {
				return false;
			}
		}
		return true;
	}

    std::string intToStr(int n)
    {
        std::ostringstream oss;
        oss << n;
        return oss.str();
    }
}
