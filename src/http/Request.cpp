#include "http/Request.hpp"

Request::Request(): content_len(0), has_content_length(false), chunked(false) {}

void Request::setMethod(const std::string& m) { method = m; }
void Request::setPath(const std::string& p) { path = p; }
void Request::setVersion(const std::string& v) { version = v; }
void Request::setHeader(const std::string& key, const std::string& value)
{
    headers[key] = value;
}
void Request::setBody(const std::string& b) { body = b; }
void Request::setContent_len(const size_t& c) { content_len = c; }
void Request::setHas_content_length(const bool& c) { has_content_length = c; }
void Request::setChunked(const bool& c) { chunked = c; }
void Request::setQuery(const std::string& q) { query = q; }


const std::string& Request::getMethod() const { return method; }
const std::string& Request::getPath() const { return path; }
const std::string& Request::getVersion() const { return version; }
const std::string& Request::getBody() const { return body; }
const std::string& Request::getQuery() const { return query; }


std::string Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return "";
}

const size_t& Request::getContent_len() const { return content_len; }
const bool& Request::getHas_content_length() const { return has_content_length; }
const bool& Request::getChunked() const { return chunked; }

std::string Request::getHeaders(void) const{
    std::string result;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it)
    {
        result += it->first + ": " + it->second + "\n";
    }
    return result;
}
