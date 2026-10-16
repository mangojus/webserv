#include "http/Request.hpp"
#include "http/HttpParser.hpp"
#include "client/Client.hpp"
#include <sys/stat.h>
#include <stdlib.h>
#include <iostream> 

bool file_exists_cp(const std::string &path)
{
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

bool is_path_safe(const std::string& path)
{
    if (path.find("..") != std::string::npos)
        return false;
    if (path.find('\0') != std::string::npos)
        return false;
    return true;
}

static void check_parent_permissions(const std::string& fs_path)
{
    std::string parent = fs_path.substr(0, fs_path.rfind('/'));
    if (parent.empty())
        parent = "/";

    struct stat st_dir;
    if (stat(parent.c_str(), &st_dir) != 0)
        throw std::runtime_error("403");

    if (!(st_dir.st_mode & S_IWOTH) || !(st_dir.st_mode & S_IXOTH))
        throw std::runtime_error("403");
}

void Client::check_permissions(const std::string& path, const struct stat& st, const std::string& method)
{
    if (hasCgiRequest() && (st.st_mode & S_IEXEC))
        return ;

    if (method == "GET" && !(st.st_mode & S_IROTH))
        throw std::runtime_error("403");

    if (method == "POST" || method == "DELETE"){
        check_parent_permissions(path);
    }
}

std::string Client::resolve_path()
{
    std::string uri = _req.getPath();
    std::string root = _selected_location->root;

    if (uri.length() > 2048)
        throw std::runtime_error("414");

    const std::string forbidden_chars = "|;&$><`\\";
    for (size_t i = 0; i < forbidden_chars.size(); i++)
    {
        if (uri.find(forbidden_chars[i]) != std::string::npos)
            throw std::runtime_error("400");
    }

    if (root.empty() || root[root.size() - 1] != '/')
        root += "/";

    if (!uri.empty() && uri[0] == '/')
        uri = uri.substr(1);

    std::string full_path = root + uri;

    if (!is_path_safe(full_path)) {
        throw std::runtime_error("403");
    }

    return full_path;
}

std::string Client::resolve_final_path(const std::string& fs_path)
{
    struct stat st;

    if (stat(fs_path.c_str(), &st) != 0)
    {
        if (_req.getMethod() == "POST")
        {
            check_parent_permissions(fs_path);
            _isdir = false;
            return fs_path;
        }
        throw std::runtime_error("404");
    }

    if (S_ISDIR(st.st_mode)){
        _isdir = true;
        return resolve_directory(fs_path);
    }
    _isdir = false;

    check_permissions(fs_path, st, _req.getMethod());

    if (!S_ISREG(st.st_mode))
        throw std::runtime_error("403");

    return fs_path;
}

std::string Client::resolve_directory(const std::string& fs_path)
{
    if (_req.getMethod() == "POST")
    {
        check_parent_permissions(fs_path);
        return fs_path;
    }


    std::string dir = fs_path;
    if (dir[dir.size() - 1] != '/')
        dir += "/";

    std::string index_name = get_index_name();
    std::string candidate = dir + index_name;

    if (!file_exists_cp(candidate)){

        if (_req.getMethod() != "GET")
            throw std::runtime_error("405");

        bool autoindex = (_selected_location && _selected_location->autoIndex);
        if (!autoindex)
            throw std::runtime_error("403");

        return fs_path;
    }

    if (!is_path_safe(candidate))
        throw std::runtime_error("403");

    struct stat st_idx;
    if (stat(candidate.c_str(), &st_idx) == 0)
        check_permissions(candidate, st_idx, _req.getMethod());

    return candidate;
}

std::string Client::get_index_name() const
{
    if (_selected_location && !_selected_location->index.empty())
        return _selected_location->index;
    if (_selected_server && !_selected_server->index.empty())
        return _selected_server->index;
    return "index.html";
}