#pragma once

#include <string>
#include <exception>
#include "http/Request.hpp"
#include "config/Config.hpp"
#include <sys/types.h>

enum CgiState
{
	IDLE = 0,
	RUNNING = 2,
	READ = 4,
	WRITE = 8,
	FINISHED = 16
};

struct Pipe
{
	int r;
	int w;

	Pipe() : r(-1), w(-1) {}
	void init();
	void closeRead();
	void closeWrite();
};

struct CgiPipes
{
	CgiPipes() : to_cgi(), from_cgi() {} // to_cgi = in from_cgi = out replace later
	Pipe to_cgi;
	Pipe from_cgi;
};

class Cgi
{
public:
	Cgi();
	~Cgi();

	void init(Request *req, const LocationConfig *locCfg);
	void execute(const std::string &fullPath, const std::string &cgiPass); // en fonction de GET ou POST
	bool checkChildStatus();

	void closePipes();

	void debug() const;

	CgiState getCgiState() const;
	std::string getInBuf() const;
	pid_t getPid() const;
	std::string getOutBuf() const; // only for debugging, should be removed later
	int getWriteFd() const { return _pipes.to_cgi.w; }
	int getReadFd() const { return _pipes.from_cgi.r; }
	CgiPipes _pipes;
	void appendOutput(const char* data, size_t size);
	size_t getBytesSent() const;
	void setBytesSent(size_t n);
	void addBytesSent(size_t n);

	// Alex CGI status handling
	void setSuccess(bool b) { _success = b; }
	void setCgiProcessComplete(bool b) { _cgiProcessComplete = b; }
	void setCgiOutputComplete(bool b) { _cgiOutputComplete = b; }
	void setExitStatus(int s) { _exitStatus = s; }
	bool getCgiProcessComplete() const { return _cgiProcessComplete; }
	bool getCgiOutputComplete() const { return _cgiOutputComplete; }
	bool getSuccess() const { return _success; }
	void closeReadFd();
	void closeWriteFd();

	std::string _outBuf;

private:
	Cgi &operator=(const Cgi &other);
	Cgi(const Cgi &copy);

	char **_envp;
	std::string _inBuf;
	std::string _method;
	pid_t _pid;
	size_t _bytes_sent;
	int _exitStatus;
	bool _cgiProcessComplete;
	bool _cgiOutputComplete;
	bool _success;
	
	CgiState _cgiState;
	Request *_request;
	const LocationConfig *_config;

	void allocEnvp(const std::vector<std::string> &envStr);
	void nullfreeEnvp();
	void buildEnvp();
};

class CgiError : public std::exception
{
public:
	CgiError(const std::string &msg) : _message(msg) {}
	virtual ~CgiError() throw() {}
	virtual const char *what() const throw() { return _message.c_str(); }

private:
	std::string _message;
};
