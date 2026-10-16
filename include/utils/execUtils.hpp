#pragma once
#include <string>

bool file_exists(const std::string& path);
bool isDirectory(const std::string& path);
bool isFile(const std::string& path);
bool valid_script(const std::string &path);