#include "engine/Engine.hpp"
#include "utils/Signals.hpp"

#include "unistd.h"
#include <sys/wait.h>
#include <iostream>

void Engine::exit_cleanly(void)
{
	size_t i = 0;
	while (i < _pfds.size())
	{
		::close(_pfds[i].fd);
		++i;
	}
	_pfds.clear();
	_fdCtx.clear();
}


void Engine::handle_waiting_signals(void)
{
	pid_t pid;
	int status;

	if (g_sig_child_dead == 1)
	{
		g_sig_child_dead = 0;
		while (1)
		{
			pid = waitpid(-1, &status, WNOHANG);
			if (pid <= 0)
				break;
			notify_child_event(pid, status);
		}
	}
}

void Engine::notify_child_event(pid_t pid, int status)
{
	std::map<pid_t, Client*>::iterator it;
	Client *client;

	it = _cgiClients.find(pid);
	if (it == _cgiClients.end())
		return;
	client = it->second;
	if (client == NULL)
	{
		_cgiClients.erase(it);
		return;
	}
	client->handle_cgi_status(status);
	if (client->getPendingCleanup())
	{
		size_t initial_size = _pfds.size();
		cleanup_client(client->getFd(), initial_size);
		client->setPendingCleanup(false);
		return;
	}
	try_finalize_cgi_response(*client);
	// std::cerr << "Warning: received SIGCHLD for pid " << pid << " but no client has this pid\n";
}

void Engine::remove_cgi_tracking(Client &client)
{
	pid_t pid = client.getCgi().getPid();

	if (pid > 0)
		_cgiClients.erase(pid);
}