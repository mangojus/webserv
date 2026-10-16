#pragma once

#include <string>
#include <map>


class CgiResponse
{
public:
	// CgiResponse();
	// ~CgiResponse();

	bool splitHeaderBody(const std::string &output, std::string &header, std::string &body);

	bool parseCgiHeader(const std::string &header, std::map<std::string, std::string> &headersMap, int& status_code, std::string& status_text);

private:

};