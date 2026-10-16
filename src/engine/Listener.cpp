#include "engine/Listener.hpp"
#include "utils/AddrInfoGuard.hpp"

#include <iostream>
#include <cstring>		// std::memset
#include <sys/socket.h> // socket
#include <cerrno>		// cerr
#include <sstream>
#include <iostream>
#include <string>

#include <unistd.h>		// close
#include <netinet/in.h> // struct sockaddr
#include <fcntl.h>		// non blocking flags
#include <sys/types.h>
#include <netdb.h> 		// getaddrinfo

Listener::Listener() : _host(""), _port(0), _server_fd(-1), _backlog(0)
{
	// should not be used
}

Listener::Listener(const std::string &host, int port) : _host(host), _port(port), _server_fd(-1), _backlog(0) {};

Listener::~Listener()
{
	if (_server_fd != -1)
		::close(_server_fd);
}

void Listener::addConfig(const Config *config)
{
	_configs.push_back(config);
	if (config->backlog > 0 && config->backlog > _backlog)
		_backlog = config->backlog;
}

void Listener::open_socket()
{
	initiate_socket();
}

std::string intToString(int n)
{
	std::stringstream ss;
	ss << n;
	return ss.str();
}

void Listener::initiate_socket()
{
	struct addrinfo hints;
	struct addrinfo *tmp;
	AddrInfoGuard addrInfoGuard;
	std::string port;
	int ret;
	const char *host;
	int optval;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;		 // AF_INET = IPv4; AF_INET6 = IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP

	port = intToString(_port);

	if (_host == "0.0.0.0")
	{
		hints.ai_flags = AI_PASSIVE;
		host = NULL; // INADDR_ANY
	}
	else
	{
		hints.ai_flags = 0;
		host = _host.c_str();
	}

	ret = getaddrinfo(host, port.c_str(), &hints, addrInfoGuard.get_ptr());
	if (ret != 0)
	{
		std::cerr << "getaddrinfo: " << gai_strerror(ret) << std::endl; // getaddrinfo does not set errno, so we use gai_strerror to get the error message
		throw std::runtime_error("getaddrinfo failed");
	}

	_server_fd = -1;
	tmp = addrInfoGuard.get();
	while (tmp != NULL)
	{
		_server_fd = ::socket(tmp->ai_family,
							  tmp->ai_socktype,
							  tmp->ai_protocol);
		if (_server_fd < 0)
		{
			tmp = tmp->ai_next;
			continue;
		}

		optval = 1;
		if (::setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) // authorize to reuse adress
		{
			::close(_server_fd);
			_server_fd = -1;
			tmp = tmp->ai_next;
			continue;
		}

		if (::bind(_server_fd, tmp->ai_addr, tmp->ai_addrlen) == 0) // == 0 means success
			break;													// success
		else
		{
			::close(_server_fd);
			_server_fd = -1;
			tmp = tmp->ai_next;
		}
	}

	if (_server_fd < 0)
		throw SystemError("socket or bind");
	
	if (::listen(_server_fd, _backlog) < 0)
		throw SystemError("listen");

	if (non_blocking(_server_fd) < 0)
		throw SystemError("non_blocking");
}

int Listener::non_blocking(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) // keep the flag + NONBLOCK
		return -1;
	return 0;
}

int Listener::getFd() const
{
	return _server_fd;
}

const std::vector<const Config *> &Listener::getConfigs() const
{
	return _configs;
}
