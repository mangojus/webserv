#include "engine/Engine.hpp"

#include "engine/Engine.hpp"

#include "engine/Listener.hpp"
#include "client/Client.hpp"

#include "poll.h"
#include <string>
#include <iostream>
#include <vector>
#include <map>

void printListener(const Listener& listener);
void printClient(const Client& client);

void Engine::debug() const
{
	std::cout << "==================== DEBUG:Engine ====================\n\n";
	int i = 0;
	std::cout << "Nb of Listeners: " << _nbListeners << "\n";
	std::cout << std::endl;

	for (std::vector<pollfd>::const_iterator it = _pfds.begin(); it != _pfds.end(); ++it) {
		std::cout << "pfd[" << i << "]:\n";
		std::cout << "fd: " << it->fd << "\n";
		std::cout << "events: " << it->events << "\n";
		std::cout << "revents: "<< it->revents << "\n";
		++i;
	}
	std::cout << std::endl;

	i = 0;
	for (std::vector<Listener*>::const_iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		std::cout << "Listener[" << i << "]:\n";
		printListener(**it);
		++i;
	}

	i = 0;
	for (std::map<int, Client *>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
		std::cout << "Client[" << i << "]: fd[" << it->first << "]\n";
		(it->second)->debug();
		++i;
	}
	std::cout << std::endl;
}
