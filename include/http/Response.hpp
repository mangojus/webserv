
#include "config/Config.hpp"
#include <string>
#include <iostream>
#include <map>

class Response
{
	public:
		Response();
		Response(const std::string& method, const std::string& fullPath, const std::string& body, const std::string& uriPath);
		Response(const std::string& errorCode);
		~Response();

		std::string serveRedirect(const std::string& statusCode, const std::string& url);
		std::string serveGet();
		std::string servePost();
		std::string serveDelete();
        std::string buildValidResponse(const std::string& code, const std::string& body);
		std::string buildErrorResponse(const Config* cfg);
		std::string findContentType(const std::string &path);
		bool checkErrorPages(const std::map<std::string, std::string>& errorPages);
		std::string buildAutoindex();

	private:
		std::string _method;
		std::string _body;
		std::string _errorCode;
		std::string _fullPath;
		std::string _uriPath;
};
