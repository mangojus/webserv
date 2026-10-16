#include "engine/Engine.hpp"
#include <iostream>

void Engine::state_sending_by_index(const size_t pfd_i, Client &client)
{
	client.setState(SENDING_RESPONSE);
	_pfds[pfd_i].events &= ~POLLIN; // we are not interested in reading anymore until we have sent the response
	_pfds[pfd_i].events |= POLLOUT;
}

/*
   we start after the servers for the clients
   for each client if there is an error we close it and remove it from the map and the poll vector
 */

void Engine::state_sending_by_fd(int client_fd, Client &client)
{
	size_t i;

	client.setState(SENDING_RESPONSE);
	i = 0;
	while (i < _pfds.size())
	{
		if (_pfds[i].fd == client_fd)
		{
			_pfds[i].events &= ~POLLIN;
			_pfds[i].events |= POLLOUT;
			return;
		}
		i++;
	}
}

void Engine::state_sending(Client &client)
{
	size_t i;
	i = 0;
	while (i < _pfds.size())
	{
		if (_pfds[i].fd == client.getFd())
		{
			_pfds[i].events &= ~POLLIN;
			_pfds[i].events |= POLLOUT;
			return;
		}
		i++;
	}
}

void Engine::advance_state(size_t pfd_i, Client &client)
{
	if (client.getState() == READING_HEADER) {
		if (!client.try_parse_header()) // not complete yet, wait for more data
			return;
	}
	if (client.getState() == READING_BODY) {
		if (!client.try_read_body())
			return;
	}
	if (client.getState() == READING_CHUNK) {
		if (!client.try_read_chunked())
			return;
	}
	if (client.getState() == REQUEST_COMPLETE) {
		client.handle_request();
	}
	if (client.getState() == CGI_WRITING || client.getState() == CGI_READING) {
		handle_cgi_poll_fds(pfd_i, client);
	}
	if (client.getState() == BUILDING_STATIC_RESPONSE) {
		client.handle_response();
	}
	if (client.getState() == ERROR_STATE) {
		client.handle_error();
	}
	if (client.getState() == SENDING_RESPONSE) {
		state_sending(client);
	}
}
