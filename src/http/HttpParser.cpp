#include "http/HttpParser.hpp"
#include "utils/StringUtils.hpp"
#include "engine/Listener.hpp"
#include <sstream>
#include <stdexcept>
#include <iostream>


Request HttpParser::parse(const std::string& rawRequest)
{
    Request req;

    parseHeadersLine(rawRequest, req);

    size_t firstLineEnd = rawRequest.find("\r\n");
    if (firstLineEnd == std::string::npos)
        throw std::runtime_error("400");

    size_t headersStart = firstLineEnd + 2;
    parseHeadersMap(rawRequest, headersStart, req);

    if (req.getHeader("host").empty())
        throw std::runtime_error("400");

    size_t bodyStart = rawRequest.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
        parseHeadersBody(req);

    return req;
}

void HttpParser::parseHeadersLine(const std::string& raw, Request& req)
{
    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos)
        throw std::runtime_error("400");

    std::string line = raw.substr(0, pos);

    if (utils::countWords(line) != 3)
        throw std::runtime_error("400");

    std::istringstream iss(line);
    std::string method, path, version;
    iss >> method >> path >> version;

    std::string extra;
    if (iss >> extra)
        throw std::runtime_error("400");

    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
    {
        req.setQuery(path.substr(qpos + 1));
        path = path.substr(0, qpos);
    }

    if (method != "GET" && method != "POST" && method != "DELETE")
        throw std::runtime_error("405");
    if (version != "HTTP/1.1" && version != "HTTP/1.0")
        throw std::runtime_error("505");
    if (path.empty() || path[0] != '/')
        throw std::runtime_error("400");

    req.setMethod(method);
    req.setPath(path);
    req.setVersion(version);
}

void HttpParser::parseHeadersMap(const std::string& raw, size_t pos, Request& req)
{
    size_t headerCount = 0;

    const size_t MAX_HEADERS = 100;
    const size_t MAX_HEADER_SIZE = 8192; // 8KB par header

    while (pos < raw.length())
    {
        size_t lineEnd = raw.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            throw std::runtime_error("400");
        std::string line = raw.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        if (++headerCount > MAX_HEADERS)
            throw std::runtime_error("431");

        if (line.length() > MAX_HEADER_SIZE)
            throw std::runtime_error("400");


        size_t colon = line.find(':');
        if (colon == std::string::npos)
            throw std::runtime_error("400");
        std::string key = utils::trim(line.substr(0, colon));
        std::string value = utils::trim(line.substr(colon + 1));
        if (value.empty())
            throw std::runtime_error("400");
        key = utils::toLower(key);
        if (!req.getHeader(key).empty() && isUniqueHeader(key))
            throw std::runtime_error("400");

        for (size_t i = 0; i < key.length(); i++)
        {
            unsigned char c = key[i];
            if (c <= 0x20 || c == 0x7F)
                throw std::runtime_error("400");
        }
        for (size_t i = 0; i < value.length(); i++)
        {
            unsigned char c = value[i];
            if (c < 0x20 || c == 0x7F)
                throw std::runtime_error("400");
        }

        req.setHeader(key, value);
        pos = lineEnd + 2;
    }
}

void HttpParser::parseHeadersBody(Request& req)
{
    std::string te = req.getHeader("transfer-encoding");
    if (!te.empty() && utils::toLower(te) == "chunked")
    {
        req.setChunked(true);
        return;
    }
    else
        req.setChunked(false);

    std::string contentLengthStr = req.getHeader("content-length");
    if ((!te.empty() && !contentLengthStr.empty()) || (req.getMethod() == "POST" && contentLengthStr.empty()))
        throw std::runtime_error("400");

    if (contentLengthStr.empty())
    {
        req.setHas_content_length(false);
        return;
    }
    else
        req.setHas_content_length(true);

    size_t contentLength = 0;
    std::istringstream iss(contentLengthStr);
    if (!(iss >> contentLength) || !iss.eof())
        throw std::runtime_error("400");

    for (size_t i = 0; i < contentLengthStr.length(); i++)
    {
        if (!isdigit(contentLengthStr[i]))
            throw std::runtime_error("400");
    }

    req.setContent_len(contentLength);
}
