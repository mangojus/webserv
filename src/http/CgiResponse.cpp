#include "http/CgiResponse.hpp"
#include "utils/StringUtils.hpp"

#include <iostream>
#include <sstream>
#include <map>

bool CgiResponse::splitHeaderBody(const std::string &output, std::string &header, std::string &body)
{
	size_t header_end = output.find("\r\n\r\n");
	if (header_end != std::string::npos)
	{
		header = output.substr(0, header_end);
		body = output.substr(header_end + 4);
	}
	else if ((header_end = output.find("\n\n")) != std::string::npos)
	{
		header = output.substr(0, header_end);
		body = output.substr(header_end + 2);
	}
	else
	{
		std::cerr << "Warning: end of headers not found in CGI output\n";
		return false;
	}
	return true;
}

bool CgiResponse::parseCgiHeader(const std::string &header, std::map<std::string, std::string> &headersMap, int &status_code, std::string &status_text)
{
	std::string line;
	std::istringstream iss(header);
	size_t colon;
	std::string key;
	std::string value;

	status_code = 200;
	status_text = "OK";

	while (std::getline(iss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r') // so we can handle /r/n same as /n
			line = line.erase(line.size() - 1);
		if (line.empty())
			continue;

		colon = line.find(':');
		if (colon == std::string::npos)
			return false;

		key = utils::trim(line.substr(0, colon));
		value = utils::trim(line.substr(colon + 1));
		if (value.empty() || key.empty())
			return false;

		key = utils::toLower(key);
		if (key == "status")
		{
			std::istringstream ss(value);
			if (!(ss >> status_code))
				return false;
			if (status_code < 100 || status_code > 599)
				return false;
			std::getline(ss, status_text);
			status_text = utils::trim(status_text);
			if (status_text.empty())
				return false;
			continue;
		}
		for (size_t i = 0; i < key.length(); i++)
		{
			unsigned char c = key[i];
			if (c <= 0x20 || c == 0x7F)
				return false;
		}
		for (size_t i = 0; i < value.length(); i++)
		{
			unsigned char c = value[i];
			if (c < 0x20 || c == 0x7F)
				return false;
		}
		headersMap[key] = value;
	}
	return true;
}