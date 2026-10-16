#include "client/Client.hpp"
#include <iostream>
#include <sys/wait.h>

std::string get_extension(const std::string &path)
{
	size_t dot = path.rfind('.');
	if (dot == std::string::npos)
		return "";
	return path.substr(dot);
}

bool Client::hasCgiRequest()
{
	if (!_selected_location->hasCgi)
		return false;

	std::string cgi_ext = _selected_location->cgiExtension;
	if (cgi_ext.empty())
		throw std::runtime_error("500"); //if location has cgi but ext empty then server problem, rshin

	if (get_extension(_req.getPath()) != cgi_ext)
		throw std::runtime_error("403"); // if extension does not match then its forbidden rshin

	return true;
}

void Client::start_cgi()
{
	updateCgiLastActivity();
	_cgi.init(&_req, _selected_location);
	_cgi.execute(_full_path, _selected_location->cgiPass);
	if (_req.getMethod() == "GET")
	{
		_state = CGI_READING;
	}
	else if (_req.getMethod() == "POST")
	{
		_state = CGI_WRITING;
	}
}

bool Client::cgiHasThisPid(pid_t pid)
{
	if (pid == _cgi.getPid())
		return true;
	return false;
}

void Client::handle_cgi_status(int status)
{
	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) != 0) // exit 1 in script
		{
			std::cerr << "Warning: CGI process exited with non-zero status " << WEXITSTATUS(status) << std::endl;
			_cgi.setSuccess(false);
		}
		else
		{
			_cgi.setSuccess(true);
		}
	}
	else if (WIFSIGNALED(status)) // SIGSEGV SIGABRT SIGFPE etc
	{
		std::cerr << "Warning: CGI process was killed by signal " << WTERMSIG(status) << std::endl;
		_cgi.setSuccess(false);
	}
	_cgi.setCgiProcessComplete(true);
	_cgi.setExitStatus(status);

}