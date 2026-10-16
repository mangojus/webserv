#include "http/Cgi.hpp"
#include "engine/Engine.hpp"
#include "http/CgiResponse.hpp"

#include <unistd.h>
#include <iostream>

void Engine::handle_cgi_error(Client *client, int fd, const size_t i, size_t &initial_size)
{
	std::map<int, FdContext>::iterator it = _fdCtx.find(fd);
	if (it != _fdCtx.end())
	{
		std::cout << "handle_cgi_error fd=" << fd
				  << " type=" << it->second.type
				  << " errno=" << errno
				  << " pid=" << client->getPid()
				  << std::endl;
	}
	client->setErrorCode("500");
	client->setState(ERROR_STATE);
	advance_state(i, *client);
	cleanup_fd_at_index(i, initial_size, fd);
}


void Engine::handle_cgi_poll_fds(const size_t client_pfd, Client &client)
{
	pid_t pid = client.getCgi().getPid();
	if (pid > 0 && _cgiClients.find(pid) == _cgiClients.end())
	{
		_cgiClients[pid] = &client;
	}

	_pfds[client_pfd].events = 0;

	if (client.getState() == CGI_WRITING) // add this =>  ( || client._cgi == NULL) ?
	{
		add_cgi_in_fd(client);
	}

	if (client.getState() == CGI_READING)
	{
		add_cgi_out_fd(client);
	}
}

void Engine::add_cgi_in_fd(Client &client)
{
	int fd = client.getCgi().getWriteFd();

	if (fd == -1 || _fdCtx.find(fd) != _fdCtx.end())
		return;

	pollfd p;
	p.fd = fd;
	p.events = POLLOUT;
	p.revents = 0;
	_pfds.push_back(p);

	FdContext ctx;
	ctx.type = FD_CGI_IN;
	ctx.listener_ptr = NULL;
	ctx.client_ptr = &client;
	_fdCtx[p.fd] = ctx;
}

void Engine::add_cgi_out_fd(Client &client)
{
	int fd = client.getCgi().getReadFd();

	if (fd == -1 || _fdCtx.find(fd) != _fdCtx.end())
		return;

	pollfd p;
	p.fd = fd;
	p.events = POLLIN;
	p.revents = 0;
	_pfds.push_back(p);

	FdContext ctx;
	ctx.type = FD_CGI_OUT;
	ctx.listener_ptr = NULL;
	ctx.client_ptr = &client;
	_fdCtx[p.fd] = ctx;
}

// HANDLE_FDS
bool Engine::handle_cgi_out_fd(size_t &i, size_t &initial_size, int fd, short revents, std::map<int, FdContext>::iterator it)
{
	Client *client;

	client = it->second.client_ptr;
	if (client == NULL)
	{
		cleanup_fd_at_index(i, initial_size, fd);
		return true;
	}
	if (revents & (POLLERR | POLLNVAL))
	{
		handle_cgi_error(client, fd, i, initial_size);
		return true;
	}
	if (revents & (POLLIN | POLLHUP)) // POLLHUP means the cgi process closed the writing end of the pipe, we should read the remaining data and then cleanup
	{
		if (read_from_cgi_fd(client, fd, i, initial_size))
			return true;
	}
	return false;
}

bool Engine::handle_cgi_in_fd(size_t &i, size_t &initial_size, int fd, short revents, std::map<int, FdContext>::iterator it)
{
	Client *client;

	client = it->second.client_ptr;
	if (client == NULL)
	{
		cleanup_fd_at_index(i, initial_size, fd);
		return true;
	}

	if (revents & (POLLERR | POLLNVAL))
	{
		handle_cgi_error(client, fd, i, initial_size);
		return true;
	}
	else if (revents & POLLHUP)
	{
		std::cerr << "cgi: POLLHUP received" << std::endl;
		cleanup_fd_at_index(i, initial_size, fd);
		if (client->getCgi().getBytesSent() < client->getRequest().getBody().size())
			handle_cgi_error(client, fd, i, initial_size);
		return true;
	}
	else if (revents & POLLOUT)
	{
		if (write_to_cgi_fd(client, fd, i, initial_size))
			return true;
	}
	return false;
}

// HANDLE_CGI_OUT_FD
bool Engine::read_from_cgi_fd(Client *client, int fd,
							  size_t &i, size_t &initial_size)
{
	char buf[2048];
	ssize_t n;

	n = ::read(fd, buf, sizeof(buf));
	if (n > 0)
	{
		client->updateCgiLastActivity(); // update last activity time on each read from cgi
		client->getCgi().appendOutput(buf, n);
		return false;
	}
	if (n == 0)
	{
		client->getCgi().setCgiOutputComplete(true);
		cleanup_fd_at_index(i, initial_size, fd);
		try_finalize_cgi_response(*client);
		return true;
	}
	// if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
	if (n < 0)
	{
		handle_cgi_error(client, fd, i, initial_size);
		return true;
	}
	return true;
}

// HANDLE_CGI_IN_FD
bool Engine::write_to_cgi_fd(Client *client, int fd, size_t &i, size_t &initial_size)
{
	ssize_t n;
	const std::string &body = client->getRequest().getBody();
	size_t remain;

	remain = body.size() - client->getCgi().getBytesSent();
	if (remain == 0)
	{
		cleanup_fd_at_index(i, initial_size, fd);
		client->setState(CGI_READING);
		add_cgi_out_fd(*client);
		return true;
	}
	n = ::write(fd,
				body.c_str() + client->getCgi().getBytesSent(),
				remain); // body.c_str() + client->getCgi().getBytesSent() -> start at + ByteSent
	if (n > 0)
	{
		client->updateCgiLastActivity(); // update last activity time on each write to cgi
		client->getCgi().addBytesSent(n);
		if (client->getCgi().getBytesSent() == body.size())
		{
			cleanup_fd_at_index(i, initial_size, fd);
			client->setState(CGI_READING);
			add_cgi_out_fd(*client);
			return true;
		}
		return false;
	}
	else
	{
		handle_cgi_error(client, fd, i, initial_size);
		return true;
	}
	return true;
}

#include <sstream>
std::string build_cgi_response(Client &client)
{
	CgiResponse cgiResponse;
	std::string header;
	std::string body;
	std::string output;
	std::map<std::string, std::string> headersMap; // rshin: this is a reference to the headers map in the request, we will fill it with the CGI response headers
	std::map<std::string, std::string>::iterator it;
	int status_code;
	std::string status_text;
	std::ostringstream response;

	output = client.getCgiOutput();

	if (!cgiResponse.splitHeaderBody(output, header, body))
		throw std::runtime_error("500");
	if (!cgiResponse.parseCgiHeader(header, headersMap, status_code, status_text))
		throw std::runtime_error("500");

	it = headersMap.find("content-type");
	if (it == headersMap.end())
		throw std::runtime_error("500");

	response << "HTTP/1.1 "
			 << status_code << " "
			 << status_text << "\r\n";
	it = headersMap.begin();
	while (it != headersMap.end())
	{
		if (it->first != "content-length" && it->first != "connection") // we will set these headers ourselves, so we ignore them if they are set by the CGI script
			response << it->first << ": " << it->second << "\r\n";
		++it;
	}
	response << "Content-Length: " << body.size() << "\r\n";
	response << "Connection: close\r\n";
	response << "\r\n";
	response << body;

	return response.str();
}

bool Engine::try_finalize_cgi_response(Client &client)
{
	try
	{

		if (client.getCgiFinalized())
			return true;
		if (client.getCgi().getCgiProcessComplete() && client.getCgi().getCgiOutputComplete())
		{
			remove_cgi_tracking(client);
			if (client.getCgi().getSuccess() == false)
			{
				throw std::runtime_error("500");
			}
			else
			{
				std::string response = build_cgi_response(client);
				client.setOutBuf(response);
			}
			client.setCgiFinalized(true);
			state_sending_by_fd(client.getFd(), client);
			return true;
		}
	}
	catch (const std::runtime_error &e)
	{
		remove_cgi_tracking(client);
		client.setState(ERROR_STATE);
		client.setErrorCode(e.what());
		advance_state(find_pfd_index(client.getFd()), client);
		return true;
	}
	return false;
}
