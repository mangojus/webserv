#pragma once

#include <string>
#include <vector>
#include "SystemError.hpp" // exceptions
#include "config/Config.hpp"

class Listener
{
public:
	Listener(const std::string &host, int port);
	~Listener();

	int getFd() const;
	const std::vector<const Config *> &getConfigs() const;

	void addConfig(const Config *config);
	int non_blocking(int fd);
	void open_socket();

	// public for debug
	std::string _host;
	int _port;

	void debug() const;

private:
	Listener();
	Listener(const Listener &copy);
	Listener &operator=(const Listener &other);

	void initiate_socket();
	std::vector<const Config *> _configs;

	int _server_fd;
	int _backlog;
};



