#include "http/Cgi.hpp"

#include <cstdio>
#include <cerrno>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>

void Pipe::init()
{
	int fd[2];

	if (pipe(fd) == -1)
	{
		throw CgiError("Pipe creation failed");
	}
	r = fd[0];
	w = fd[1];
}

void Pipe::closeRead()
{
	if (r != -1)
	{
		::close(r);
		r = -1;
	}
}

void Pipe::closeWrite()
{
	if (w != -1)
	{
		::close(w);
		w = -1;
	}
}

void Cgi::closePipes()
{
	_pipes.to_cgi.closeRead();
	_pipes.to_cgi.closeWrite();
	_pipes.from_cgi.closeRead();
	_pipes.from_cgi.closeWrite();
}

Cgi::Cgi() :
	_outBuf(""),
	_envp(NULL), 
	_pid(-1),
	_bytes_sent(0), 
	_exitStatus(-1),
	_cgiProcessComplete(false),
	_cgiOutputComplete(false),
	_success(false),
	_cgiState(IDLE),
	_request(NULL),
	_config(NULL) {}

Cgi::~Cgi()
{
	nullfreeEnvp();
	closePipes();
}

void Cgi::init(Request *req, const LocationConfig *locCfg)
{
	if (!req || !locCfg)
	{
		throw CgiError("missing request or config");
	}
	_request = req;
	_config = locCfg;
	buildEnvp();
	_pipes.to_cgi.init();
	_pipes.from_cgi.init();
	_method = _request->getMethod();
}

void Cgi::allocEnvp(const std::vector<std::string> &envStr)
{
	char **tmp = new char *[envStr.size() + 1];
	for (size_t i = 0; i < envStr.size(); ++i)
	{
		tmp[i] = new char[envStr[i].size() + 1];
		std::strcpy(tmp[i], envStr[i].c_str());
	}
	tmp[envStr.size()] = NULL;
	_envp = tmp;
}

void Cgi::nullfreeEnvp()
{
	if (_envp)
	{
		for (size_t i = 0; _envp[i]; ++i)
		{
			delete[] _envp[i];
		}
		delete[] _envp;
		_envp = NULL;
	}
}

void Cgi::buildEnvp()
{
	std::string hostHeader = _request->getHeader("host");
	size_t colon = hostHeader.find(':');
	std::string host;
	std::string port;
	if (colon != std::string::npos)
	{
		host = hostHeader.substr(0, colon);
		port = hostHeader.substr(colon + 1);
	}
	else
	{
		host = hostHeader;
		port = "80";
	}
	std::vector<std::string> envStr;
	envStr.push_back("SERVER_NAME=" + host);
	envStr.push_back("SERVER_PORT=" + port);
	envStr.push_back("REQUEST_METHOD=" + _request->getMethod());
	envStr.push_back("SCRIPT_NAME=" + _request->getPath());
	envStr.push_back("SERVER_PROTOCOL=" + _request->getVersion());
	envStr.push_back("QUERY_STRING=" + _request->getQuery());
	std::stringstream oss;
	oss << _request->getContent_len();
	envStr.push_back("CONTENT_LENGTH=" + oss.str());
	envStr.push_back("CONTENT_TYPE=" + _request->getHeader("content_type"));
	allocEnvp(envStr);
}

void Cgi::execute(const std::string &fullPath, const std::string &cgiPass) // en fonction de GET ou POST
{
	_pid = fork();
	if (_pid == -1)
	{
		perror("fork failed");
		return;
	}
	if (_pid == 0)
	{

		if (dup2(_pipes.to_cgi.r, STDIN_FILENO) == -1)	// cgi read from the parent
		{
			perror("dup2 stdin failed");
			closePipes();
			nullfreeEnvp();
			_exit(1);
		}
		if (dup2(_pipes.from_cgi.w, STDOUT_FILENO) == -1) // cgi write to the parent
		{
			closePipes();
			nullfreeEnvp();
			perror("dup2 stdout failed");
			_exit(1);
		}
		closePipes();
		char *argv[] = {const_cast<char *>(cgiPass.c_str()), const_cast<char *>(fullPath.c_str()), NULL};
		if (execve(argv[0], argv, _envp) == -1)
		{
			perror("execve failed");
			nullfreeEnvp();
			_exit(1);
		}
	}
	_cgiState = RUNNING;
	if (_method == "GET") {
		_pipes.to_cgi.closeRead();
		_pipes.to_cgi.closeWrite();
		_pipes.from_cgi.closeWrite();
	}
	else if (_method == "POST") {
		_pipes.to_cgi.closeRead();
		_pipes.from_cgi.closeWrite();
	}
}

bool Cgi::checkChildStatus()
{
	if (_cgiState == FINISHED)
	{
		return true;
	}
	int status;
	pid_t result = waitpid(_pid, &status, WNOHANG);

	if (result == _pid)
	{
		_cgiState = FINISHED;
	}
	else if (result == -1)
	{
		throw CgiError("child doesn't exist");
		return true;
	}
	return false;
}

void Cgi::appendOutput(const char* data, size_t size)
{
	_outBuf.append(data, size);
}

void Cgi::addBytesSent(size_t n)
{
	_bytes_sent += n;
}

void Cgi::setBytesSent(size_t n)
{
	_bytes_sent = n;
}

void Cgi::closeReadFd()
{
	_pipes.to_cgi.closeRead();
}

void Cgi::closeWriteFd()
{
	_pipes.to_cgi.closeWrite();
}


size_t Cgi::getBytesSent() const { return _bytes_sent; }
CgiState Cgi::getCgiState() const { return _cgiState; }
std::string Cgi::getOutBuf() const { return _outBuf; }
std::string Cgi::getInBuf() const { return _inBuf; }
pid_t Cgi::getPid() const { return _pid; }
