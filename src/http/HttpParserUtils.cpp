#include "http/HttpParser.hpp"

bool HttpParser::isUniqueHeader(const std::string& key)
{
    return (
        key == "host"                  ||
        key == "content-length"        ||
        key == "transfer-encoding"     ||
        key == "content-type"          ||
        key == "content-range"         ||
        key == "content-location"      ||
        key == "content-md5"           ||
        key == "date"                  ||
        key == "expect"                ||
        key == "from"                  ||
        key == "if-match"              ||
        key == "if-modified-since"     ||
        key == "if-none-match"         ||
        key == "if-range"              ||
        key == "if-unmodified-since"   ||
        key == "location"              ||
        key == "max-forwards"          ||
        key == "referer"               ||
        key == "retry-after"           ||
        key == "server"                ||
        key == "user-agent"            ||
        key == "authorization"         ||
        key == "proxy-authorization"   ||
        key == "etag"                  ||
        key == "last-modified"         ||
        key == "age"                   ||
        key == "expires"               ||
        key == "range"
    );
}