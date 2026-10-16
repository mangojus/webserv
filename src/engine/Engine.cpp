#include "engine/Engine.hpp"
#include "engine/Listener.hpp"
#include "client/Client.hpp"
#include "http/HttpParser.hpp"
#include "http/Request.hpp"
#include "utils/Signals.hpp"

#include <iostream>
#include <map>

#include <sys/socket.h> // accept
#include <unistd.h>		// close
#include <sys/wait.h>	// waitpid

Engine::Engine(const std::vector<Listener *> &listeners) : _listeners(listeners), _nbListeners(0)
{
	fill_poll_listeners();
	std::cout << "There is " << _nbListeners << " servers in poll\n\n";
	for (size_t i = 0; i < _nbListeners; ++i)
	{
		std::cout << "Server " << i << " fd=" << _pfds[i].fd << std::endl;
		std::cout << "Listening on " << _listeners[i]->_host << ":" << _listeners[i]->_port << std::endl;
		std::cout << std::endl;
	}
	std::cout << "Engine initialized\n";
}

Engine::~Engine()
{
	for (size_t i = 0; i < _pfds.size(); ++i)
	{
		close(_pfds[i].fd);
	}
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		delete it->second;
	}
}

bool Engine::handle_client_send(const int fd, size_t &initial_size)
{

	std::map<int, Client *>::iterator it;
	Client *client;
	ssize_t n;

	it = _clients.find(fd);
	if (it == _clients.end())
		return false;

	client = it->second;
	n = ::send(fd, client->getOutBuf().c_str(), client->getOutBuf().size(), 0);
	if (n <= 0)
	{
		if (client_has_running_cgi(*client))
			abort_client_running_cgi(*client, initial_size);
		else
			cleanup_client(fd, initial_size);
		return true;
	}
	client->updateLastActivity(); // update last activity time on each send
	client->consumeSentData(n);

	if (client->getOutBuf().empty())
	{
		cleanup_client(fd, initial_size);
		return true;
	}
	return false;
}

bool Engine::handle_client_recv(const int fd, const size_t index, size_t &initial_size)
{
	char buf[2048];
	std::map<int, Client *>::iterator it = _clients.find(fd);

	if (it == _clients.end())
		return false;
	Client *client = it->second;
	ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
	if (n > 0)
	{
		client->updateLastActivity();		// update last activity time on each recv
		client->appendIncomingData(buf, n); // rshin:client append method
		if (client->getInBuf().size() > _maxRequestSize)
		{
			client->setErrorCode("413");
			client->setState(ERROR_STATE);
			advance_state(index, *client);
			return false;
		}
	}
	else if (n <= 0)
	{

		if (client_has_running_cgi(*client))
			abort_client_running_cgi(*client, initial_size);
		else
			cleanup_client(fd, initial_size);
		return true;
	}

	if (client->getState() == READING_HEADER ||
		client->getState() == READING_BODY ||
		client->getState() == READING_CHUNK ||
		client->getState() == ERROR_STATE)
	{
		advance_state(index, *client);
	}
	return false;
}

bool Engine::handle_client_fd(const size_t &i, size_t &initial_size, int fd, short revents)
{
	std::map<int, Client *>::iterator it;

	it = _clients.find(fd);
	if (it == _clients.end())
		return false;

	if (revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		if (client_has_running_cgi(*it->second))
			abort_client_running_cgi(*it->second, initial_size);
		else
			cleanup_client(fd, initial_size);
		initial_size = _pfds.size();
		return true;
	}

	if (revents & POLLIN)
	{
		if (handle_client_recv(fd, i, initial_size))
		{
			initial_size = _pfds.size();
			return true;
		}
	}
	else if (revents & POLLOUT)
	{
		if (handle_client_send(fd, initial_size))
		{
			initial_size = _pfds.size();
			return true;
		}
		initial_size = _pfds.size();
	}
	return false;
}

// _pfds[i] represent a fd with a potentiel event
// _fdCtx[_pfds[i]] give us the context of this fd (is it a listener, a client, or a cgi pipe)
void Engine::handle_fds()
{
	size_t i;
	size_t initial_size;
	int fd;
	short revents;
	std::map<int, FdContext>::iterator it;

	i = 0;
	initial_size = _pfds.size();

	while (i < initial_size)
	{
		fd = _pfds[i].fd;
		revents = _pfds[i].revents;

		if (revents == 0) // no event, skip this fd
		{
			++i;
			continue;
		}

		it = _fdCtx.find(fd);
		if (it == _fdCtx.end()) // if ever we deleted the context but not yet from pfds
		{
			++i;
			continue;
		}

		// if one handler return false it means it needs to treated later so we keep it with ++i
		// handlers that return true means they have treated the event and we should not increment i because we removed the fd at index i and the next fd is now at index i
		if (it->second.type == FD_LISTENER)
		{
			accept_clients(it->second.listener_ptr);
		}
		else if (it->second.type == FD_CLIENT)
		{
			if (it->second.client_ptr == NULL)
			{
				cleanup_client(fd, initial_size);
				continue;
			}
			if (handle_client_fd(i, initial_size, fd, revents))
				continue;
		}
		else if (it->second.type == FD_CGI_IN)
		{
			if (handle_cgi_in_fd(i, initial_size, fd, revents, it))
				continue;
		}
		else if (it->second.type == FD_CGI_OUT)
		{
			if (handle_cgi_out_fd(i, initial_size, fd, revents, it))
				continue;
		}
		++i;
	}
}

bool Engine::run()
{
	setup_signal();
	while (g_sig_stop == 0)
	{
		handle_waiting_signals();
		if (!wait_events()) // poll()
			return false;
		timeout_checker();
		handle_waiting_signals();
		handle_fds();
	}
	print_fds();
	exit_cleanly();
	std::cout << "pfd size after cleanup: " << _pfds.size() << std::endl;
	return true;
}

void Engine::print_fds()
{
	std::cout << "At exit:\n";
	std::map<int, FdContext>::const_iterator it;

	std::cout << "ctx0 == LISTENER ctx1 == CLIENT ctx2 == CGI_IN ctx3 = CGI_OUT" << std::endl;
	for (size_t i = 0; i < _pfds.size(); ++i)
	{
		it = _fdCtx.find(_pfds[i].fd);
		std::cout << "PFDS[" << i << "]: fd=" << _pfds[i].fd << " ctx" << it->second.type << std::endl;
	}
}