#pragma once
#include <string>
#include "Request.hpp"
#include "engine/Listener.hpp"


class HttpParser
{
private:
    void parseHeadersLine(const std::string& raw, Request& req);
    void parseHeadersMap(const std::string& raw, size_t startPos, Request& req);
    void parseHeadersBody(Request& req);
    bool isUniqueHeader(const std::string& key);

public:
    Request parse(const std::string& rawRequest);
};
