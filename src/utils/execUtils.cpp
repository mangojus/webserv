#include "utils/execUtils.hpp"

#include <string>
#include <sys/stat.h>

bool file_exists(const std::string& path)
{
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

bool isDirectory(const std::string& path)
{
    struct stat info;

    if (stat(path.c_str(), &info) != 0)
        return false;

    return S_ISDIR(info.st_mode);
}

bool isFile(const std::string& path)
{
    struct stat info;

    if (stat(path.c_str(), &info) != 0)
        return false;

    return S_ISREG(info.st_mode);
}

bool valid_script(const std::string &path)
{
	struct stat st;

	if (stat(path.c_str(), &st) != 0)
		return false;

	if (!S_ISREG(st.st_mode))
		return false;

	return true;
}