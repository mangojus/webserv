#include "client/Client.hpp"
#include "engine/Listener.hpp"
#include "http/Response.hpp"
#include "http/Request.hpp"


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>	  // read, write, close
#include <sys/stat.h> // stat
#include <cstdio>	  // remove
#include <ctime>

Client::Client()
			: _owner(NULL)
			  , _fd(-1)
			  , _state(READING_HEADER)
			  , _errorCode("")
			  , _headers_ends(0)
			  , _body_start_pos(0)
			  , _expected_body_len(0)
			  , _chunk_pos(0)
			  , _chunk_remaining(0)
			  , _header_parsed(false)
			  , _cgiFinalized(false)
			  , _pending_cleanup(false)
			  , _selected_server(NULL)
			  , _selected_location(NULL)
			  , _last_activity(std::time(NULL))
			  , _cgi_last_activity(0)
			  {};

Client::Client(const int client_fd, const Listener* listener)
	: _owner(listener)
		, _fd(client_fd)
		, _state(READING_HEADER)
		, _errorCode("")
		, _headers_ends(0)
		, _body_start_pos(0)
		, _expected_body_len(0)
		, _chunk_pos(0)
		, _chunk_remaining(0)
		, _header_parsed(false)
		, _cgiFinalized(false)
		, _pending_cleanup(false)
		, _selected_server(NULL)
		, _selected_location(NULL)
		, _last_activity(std::time(NULL))
		, _cgi_last_activity(0)
		{};

Client::~Client() {
	if (_fd != -1)
		close(_fd);
}

void Client::closeFd() {
	if (_fd != -1)
	{
		close(_fd);
		_fd = -1;
	}
}

void Client::appendIncomingData(const char *data, size_t size)
{
	_in_buffer.append(data, size);
}

void Client::consumeSentData(size_t bytes)
{
	_out_buffer.erase(0, bytes);
}



void Client::setOwner(Listener* listener) { _owner = listener; }
void Client::setFd(const int client_fd) { _fd = client_fd; }
void Client::setState(const State state) { _state = state; }
void Client::setErrorCode(std::string errorCode) { _errorCode = errorCode; }
void Client::setInBuf(const std::string& in_buffer) { _in_buffer = in_buffer; }
void Client::setOutBuf(const std::string& out_buffer) { _out_buffer = out_buffer; }

Request& Client::getRequest() { return _req; }
Cgi& Client::getCgi() { return _cgi; }

std::string Client::getFullPath() const { return _full_path; }
std::string Client::getInBuf() const { return _in_buffer; }
std::string Client::getOutBuf() const { return _out_buffer; }
std::string Client::getDecodedBody() const { return _decoded_body; }

int Client::getFd() const { return _fd; }
State Client::getState() const { return _state; }
const std::string Client::getErrorCode() const { return _errorCode; }

pid_t Client::getPid() const { return _cgi.getPid(); }

bool Client::getCgiHasExited() const { return _cgi.getCgiProcessComplete(); }
bool Client::getCgiFinalized() const { return _cgiFinalized; }
void Client::setCgiFinalized(bool b) { _cgiFinalized = b; }
std::string Client::getCgiOutput() const { return _cgi._outBuf; }
bool Client::getPendingCleanup() const { return _pending_cleanup; }
void Client::setPendingCleanup(bool b) { _pending_cleanup = b; }
