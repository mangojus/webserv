#include "http/Response.hpp"
#include "config/Config.hpp"
#include "utils/execUtils.hpp"
#include <sstream>
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>

Response::Response() {}

Response::Response(const std::string& method, const std::string& fullPath, const std::string& body, const std::string& uriPath)
	: _method(method)
	  , _body(body)
	  , _fullPath(fullPath)
	  , _uriPath(uriPath)
{}

Response::Response(const std::string& errorCode) : _errorCode(errorCode) {}

Response::~Response() {}

std::string Response::findContentType(const std::string &path)
{
	static std::map<std::string, std::string> type;
	if (type.empty())
	{
		type[".html"] = "text/html; charset=UTF-8";
		type[".htm"]  = "text/html; charset=UTF-8";
		type[".css"]  = "text/css";
		type[".js"]   = "application/javascript";
		type[".json"] = "application/json";
		type[".xml"]  = "application/xml";
		type[".txt"]  = "text/plain; charset=UTF-8";
		type[".png"]  = "image/png";
		type[".jpg"]  = "image/jpeg";
		type[".jpeg"] = "image/jpeg";
		type[".gif"]  = "image/gif";
		type[".svg"]  = "image/svg+xml";
		type[".ico"]  = "image/x-icon";
		type[".pdf"]  = "application/pdf";
		type[".zip"]  = "application/zip";
	}

	size_t dot = path.rfind('.');
	if (dot != std::string::npos)
	{
		std::map<std::string, std::string>::iterator it = type.find(path.substr(dot));
		if (it != type.end())
			return it->second;
	}

	// Fallback : flux binaire brut
	return "application/octet-stream";
}

std::string Response::buildValidResponse(const std::string& code, const std::string &body)
{
	static std::map<std::string, std::string> statuses;
	if (statuses.empty())
	{
		statuses["200"] = "OK";
		statuses["201"] = "Created";
		statuses["204"] = "No Content";
		statuses["301"] = "Moved Permanently";
		statuses["302"] = "Found";
		statuses["304"] = "Not Modified";
	}

	std::string reason = "OK";
	std::map<std::string, std::string>::iterator it = statuses.find(code);
	if (it != statuses.end())
		reason = it->second;

	std::ostringstream res;
	std::string contentType;
	struct stat st;

	if (stat(_fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)){
		std::string contentType = "text/html; charset=UTF-8";
	}
	else 
		std::string contentType = findContentType(_fullPath);
	
	res << "HTTP/1.1 " << code << " " << reason << "\r\n";
	res << "Content-Type: " << contentType << "\r\n";
	res << "Content-Length: " << body.size() << "\r\n";
	res << "Connection: close\r\n";
	res << "\r\n";
	res << body;

	return res.str();
}

bool Response::checkErrorPages(const std::map<std::string, std::string>& errorPages)
{
	const std::map<std::string, std::string>::const_iterator it = errorPages.find(_errorCode);	
	if (it == errorPages.end()) {
		return false;
	}
	_fullPath = it->second;
	return true;
}

std::string Response::buildErrorResponse(const Config* cfg)
{
	std::string reason;
	bool hasErrorPage = false;

	if (cfg){
		if (checkErrorPages(cfg->errorPages)) {
			_fullPath = cfg->root + "/" + _fullPath;
			std::ifstream file(_fullPath.c_str(), std::ios::in | std::ios::binary);
			if (file.is_open()) {
				std::stringstream buffer;
				buffer << file.rdbuf();
				_body = buffer.str();
				hasErrorPage = true;
			}
			else {
				_errorCode = "404";
			}
		}
	}
	static std::map<std::string, std::string> errors;
	if (errors.empty())
	{
		errors["400"] = "Bad Request";
		errors["401"] = "Unauthorized";
		errors["403"] = "Forbidden";
		errors["404"] = "Not Found";
		errors["405"] = "Method Not Allowed";
		errors["408"] = "Request Timeout";
		errors["409"] = "Conflict";
		errors["410"] = "Gone";
		errors["413"] = "Payload Too Large";
		errors["414"] = "URI Too Long";
		errors["415"] = "Unsupported Media Type";
		errors["429"] = "Too Many Requests";
		errors["500"] = "Internal Server Error";
		errors["501"] = "Not Implemented";
		errors["502"] = "Bad Gateway";
		errors["503"] = "Service Unavailable";
		errors["504"] = "Gateway Timeout";
		errors["505"] = "HTTP Version Not Supported";
	}

	std::map<std::string, std::string>::iterator it = errors.find(_errorCode);

	if (it != errors.end())
		reason = it->second;
	else
	{
		_errorCode = "400";
		reason = "Bad Request";
	}
	if (_body.empty()) {
		_body = _errorCode + " " + reason;
	}

	std::ostringstream res;
	std::string contentType = findContentType(_fullPath);
	res << "HTTP/1.1 " << _errorCode << " " << reason << "\r\n";
	if (hasErrorPage) {
		res << "Content-Type: text/html\r\n";
	}
	else {
		res << "Content-Type: text/plain\r\n";
	}
	res << "Content-Length: " << _body.size() << "\r\n";
	res << "Connection: close\r\n";
	res << "\r\n";
	res << _body;

	return res.str();
}

std::string Response::buildAutoindex()
{
    DIR* dir = opendir(_fullPath.c_str());
    if (!dir)
        throw std::runtime_error("403");

    const std::string style =
        "<style>\n"
        "  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "  body { font-family: monospace; background: #1e1e2e; color: #cdd6f4; padding: 40px; }\n"
        "  h1 { font-size: 1.2rem; color: #89b4fa; margin-bottom: 24px;\n"
        "       border-bottom: 1px solid #313244; padding-bottom: 12px; }\n"
        "  table { width: 100%; border-collapse: collapse; }\n"
        "  thead tr { color: #6c7086; font-size: 0.8rem; text-transform: uppercase; letter-spacing: 0.05em; }\n"
        "  thead th { text-align: left; padding: 6px 12px; }\n"
        "  tbody tr { border-top: 1px solid #313244; }\n"
        "  tbody tr:hover { background: #313244; }\n"
        "  td { padding: 8px 12px; font-size: 0.9rem; }\n"
        "  a { color: #89dceb; text-decoration: none; }\n"
        "  a:hover { color: #cba6f7; text-decoration: underline; }\n"
        "  .dir { color: #89b4fa; }\n"
        "  .size { color: #6c7086; text-align: right; }\n"
        "  .back { color: #f38ba8; }\n"
        "</style>\n";

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<meta charset=\"utf-8\">\n"
         << "<title>Index of " << _uriPath << "</title>\n"
         << style
         << "</head>\n<body>\n"
         << "<h1>Index of " << _uriPath << "</h1>\n"
         << "<table>\n"
         << "  <thead><tr><th>Name</th><th>Size</th></tr></thead>\n"
         << "  <tbody>\n";

    if (_uriPath != "/")
        html << "    <tr><td><a class=\"back\" href=\"../\">../</a></td>"
             << "<td class=\"size\">-</td></tr>\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        struct stat st;
        std::string fullEntry = _fullPath + "/" + name;
        if (stat(fullEntry.c_str(), &st) != 0)
            continue;

        bool isDir = S_ISDIR(st.st_mode);

        std::string href = _uriPath;
        if (href.empty() || href[href.size() - 1] != '/')
            href += "/";
        href += name;
        if (isDir)
            href += "/";

        std::string displayName = isDir ? name + "/" : name;
        std::string cssClass    = isDir ? "dir" : "";

        std::string sizeStr = "-";
        if (!isDir)
        {
            long size = st.st_size;
            std::ostringstream ss;
            if (size < 1024)
                ss << size << " B";
            else if (size < 1024 * 1024)
                ss << size / 1024 << " KB";
            else
                ss << size / (1024 * 1024) << " MB";
            sizeStr = ss.str();
        }

        html << "    <tr>"
             << "<td><a class=\"" << cssClass << "\" href=\"" << href << "\">"
             << displayName << "</a></td>"
             << "<td class=\"size\">" << sizeStr << "</td>"
             << "</tr>\n";
    }
    closedir(dir);

    html << "  </tbody>\n</table>\n</body>\n</html>\n";
    return html.str();
}

std::string Response::serveGet()
{
	struct stat st;

	if (stat(_fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)){
		std::string buffer = buildAutoindex();
		return buildValidResponse("200", buffer);
	}

	std::ifstream file(_fullPath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("404");

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buildValidResponse("200", buffer.str());
}

std::string Response::serveRedirect(const std::string& statusCode, const std::string& url)
{
    std::ostringstream oss;

    oss << "HTTP/1.1 " << statusCode << " Moved\r\n";
    oss << "Location: " << url << "\r\n";
    oss << "Content-Length: 0\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";

    return oss.str();
}

std::string generateTimestamp()
{
    time_t now = time(0);
    std::ostringstream oss;
    oss << now;
    return oss.str();
}

std::string Response::servePost()
{
    struct stat st;

    std::string target_path = _fullPath;

    if (stat(target_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        if (target_path[target_path.size() - 1] != '/')
            target_path += "/";
        target_path += "upload_" + generateTimestamp();
    }

    bool exists = (stat(target_path.c_str(), &st) == 0);

    std::ofstream file(target_path.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("403");

    file << _body;
    file.close();

    if (!exists)
        return buildValidResponse("201", "Created");
    return buildValidResponse("200", "Updated");
}

std::string Response::serveDelete()
{
	if (!file_exists(_fullPath))
		throw std::runtime_error("404");
	if (remove(_fullPath.c_str()) != 0)
		throw std::runtime_error("403");

	return buildValidResponse("200", "File deleted");
}

