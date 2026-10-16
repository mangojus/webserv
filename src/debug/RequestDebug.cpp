#include "http/Request.hpp"

#include <string>
#include <iostream>

void Request::debug() const
{
    std::cout << "\n=== Request: ===\n\n";
    std::cout << "Method: " << getMethod() << "\n";
    std::cout << "Path: " << getPath() << "\n";
    std::cout << "Query: " << getQuery() << "\n";
    std::cout << "Version: " << getVersion() << "\n";
    std::cout << "Body: " << getBody() << "\n";
    std::cout << "Has content length: " << getHas_content_length() << "\n";
    std::cout << "Content length: " << getContent_len() << "\n";
    std::cout << "Has chunked: " << getChunked() << "\n";
    std::cout << "Headers:\n" << getHeaders() << "\n";
    std::cout << std::endl;
}
