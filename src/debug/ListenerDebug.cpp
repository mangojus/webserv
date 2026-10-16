#include "engine/Listener.hpp"

#include <string>
#include <iostream>

void Listener::debug() const
{
	std::cout << "\n=== Listener: ===\n\n";
	std::cout << "Listener host: " << _host << "\n";
	std::cout << "Listener port: " << _port << "\n";
	std::cout << "Server fd: " << getFd() << "\n";
	std::cout << std::endl;
	//  std::cout << "Backlog: " << listener.getBacklog() << "\n";
}
