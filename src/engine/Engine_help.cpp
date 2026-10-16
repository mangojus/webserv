#include "engine/Engine.hpp"
#include "engine/Listener.hpp"
#include "client/Client.hpp"
#include "http/HttpParser.hpp"

#include <iostream>
#include <sys/socket.h> // accept
#include <unistd.h>		// close
#include <signal.h> 	// kill

void Engine::fill_poll_listeners()
{
	std::size_t i = 0;
	pollfd p;

	while (i < _listeners.size())
	{
		p.fd = _listeners[i]->getFd();
		p.events = POLLIN; // what we are looking for
		p.revents = 0;	   // not so usefull but better to do it, overwritten by poll() itself
		_pfds.push_back(p);

		FdContext ctx;
		ctx.type = FD_LISTENER;
		ctx.listener_ptr = _listeners[i];
		ctx.client_ptr = NULL;
		_fdCtx[p.fd] = ctx; // map
		++i;
	}
	_nbListeners = i;
}

bool Engine::wait_events()
{
	if (_pfds.empty())
	{
		std::cerr << "no servers to listen to\n";
		return false;
	}

	int poll_ret = ::poll(&_pfds[0], _pfds.size(), 100); // poll listen to all the servers in the vector
	if (poll_ret < 0)
	{
		if (errno == EINTR) // interrupt by signal
			return true;
		else // fatal
		{
			return false;
		}
	}
	return true;
}

void Engine::accept_clients(Listener *listener)
{
	while (1)
	{
		int client_fd = ::accept(listener->getFd(), NULL, NULL);
		if (client_fd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) // nothing in waitlist we can leave the loop
				break;
			else if (errno == EINTR) // it is fine
				continue;
			else // log it and stop accepting on this listener for now
			{
				std::cerr << "accept error: fd=" << listener->getFd() << " errno= " << std::strerror(errno) << std::endl;
				break;
			}
		}
		new_client(client_fd, listener);
	}
}

void Engine::new_client(const int client_fd, Listener *listener)
{
	if (listener->non_blocking(client_fd) < 0)
	{
		::close(client_fd);
		return;
	}

	Client *c = new Client(client_fd, listener);
	_clients[client_fd] = c;					 // store the pointer in the map

	pollfd p;
	p.fd = client_fd;
	p.events = POLLIN;
	p.revents = 0;
	_pfds.push_back(p);

	FdContext ctx;
	ctx.type = FD_CLIENT;
	ctx.client_ptr = c;
	ctx.listener_ptr = NULL;
	_fdCtx[p.fd] = ctx;
}

void Engine::cleanup_fd_by_value(int fd, size_t &initial_size)
{
	size_t i = find_pfd_index(fd);
	if (i < _pfds.size())
		cleanup_fd_at_index(i, initial_size, fd);
}

void Engine::cleanup_fd_at_index(const size_t i, size_t &initial_size, int fd)
{
	std::map<int, FdContext>::iterator	it;

	it = _fdCtx.find(fd);
	if (it != _fdCtx.end())
	{
		if (it->second.type == FD_CLIENT && it->second.client_ptr != NULL)
			it->second.client_ptr->closeFd();
		else if (it->second.type == FD_CGI_IN && it->second.client_ptr != NULL)
			it->second.client_ptr->getCgi().closeWriteFd();
		else if (it->second.type == FD_CGI_OUT && it->second.client_ptr != NULL)
			it->second.client_ptr->getCgi().closeReadFd();
		else
			::close(fd);
		_fdCtx.erase(it);
	}
	else
	{
		::close(fd);
	}
	_pfds.erase(_pfds.begin() + i);
	initial_size = _pfds.size();
}

void Engine::cleanup_client(int client_fd, size_t &initial_size)
{
	std::map<int, Client *>::iterator it;
	Client *client;
	int cgi_in_fd;
	int cgi_out_fd;

	it = _clients.find(client_fd);
	if (it == _clients.end())
		return;
	client = it->second;
	if (client == NULL)
	{
		_clients.erase(it);
		return;
	}
	cgi_in_fd = client->getCgi().getWriteFd();
	cgi_out_fd = client->getCgi().getReadFd();

	remove_cgi_tracking(*client);
	if (cgi_in_fd >= 0 && cgi_in_fd != client_fd)
		cleanup_fd_by_value(cgi_in_fd, initial_size);
	if (cgi_out_fd >= 0 && cgi_out_fd != client_fd)
		cleanup_fd_by_value(cgi_out_fd, initial_size);
	cleanup_fd_by_value(client_fd, initial_size);
	delete client;
	_clients.erase(it);
}

void Engine::clean_client_socket_only(Client &client, size_t &initial_size)
{
	int client_fd = client.getFd();
	;
	cleanup_fd_by_value(client_fd, initial_size);
}

void Engine::abort_client_running_cgi(Client &client, size_t &initial_size)
{
	if (client.getPendingCleanup())
		return;
	client.setPendingCleanup(true);
	if (client.getPid() > 0)
	{
		kill(client.getPid(), SIGKILL);
	}
	clean_client_socket_only(client, initial_size);
}

bool Engine::is_client_timeout(Client &client, time_t time)
{
	if (client.getState() == CGI_READING || client.getState() == CGI_WRITING)
	{
		if (time - client.getCgiLastActivity() > TIME_OUT)
			return true;
	}
	else
	{
		if (time - client.getLastActivity() > TIME_OUT)
			return true;
	}
	return false;
}

bool Engine::client_has_running_cgi(Client &client) const
{
	return (client.getPid() > 0 && client.getCgi().getCgiProcessComplete() == false);
}

void Engine::handle_timeout(Client &client)
{
	if (client.getState() == CGI_READING || client.getState() == CGI_WRITING)
	{
		if (client.getPid() > 0)
		{
			kill(client.getPid(), SIGKILL);
		}
		client.setState(ERROR_STATE);
		client.setErrorCode("504");
	}
	else
	{
		client.setState(ERROR_STATE);
		client.setErrorCode("408");
	}
	client.updateLastActivity();
}

void Engine::timeout_checker()
{
	time_t now = time(NULL);
	std::map<int, Client *>::iterator it;
	std::map<int, Client *>::iterator tmp;
	size_t pfd_i;

	it = _clients.begin();
	while (it != _clients.end())
	{
		tmp = it;
		tmp++;
		Client &client = *it->second;
		if (is_client_timeout(client, now))
		{
			handle_timeout(client);
			pfd_i = find_pfd_index(it->first);
			if (pfd_i < _pfds.size())
				advance_state(pfd_i, client);
		}
		it = tmp;
	}
}

size_t Engine::find_pfd_index(int fd) const
{
	size_t i;

	i = 0;
	while (i < _pfds.size())
	{
		if (_pfds[i].fd == fd)
			return i;
		i++;
	}
	return _pfds.size();
}