
#pragma once

#include <vector>
#include <map>
#include <poll.h>  // pollfd
#include <cstddef> // size_t
#include <http/Cgi.hpp>

#include "client/Client.hpp"
#include "engine/Listener.hpp"

class Listener;
class Client;

enum FdType
{
	FD_LISTENER,
	FD_CLIENT,
	FD_CGI_IN,
	FD_CGI_OUT
};

struct FdContext
{
	FdType type;
	Listener *listener_ptr;
	Client *client_ptr;
};

class Engine
{
public:
	Engine(const std::vector<Listener *> &listeners);
	~Engine();
	bool run();
	void debug() const;

private:
	Engine();

	static const long TIME_OUT = 5;

	/*RUN*/
	bool wait_events();
	void handle_fds();

	/*HANDLE FDS*/
	void accept_clients(Listener *listener);
	bool handle_client_fd(const size_t &i, size_t &initial_size, int fd, short revents);
	bool handle_cgi_in_fd(size_t &i, size_t &initial_size, int fd, short revents, std::map<int, FdContext>::iterator it);
	bool handle_cgi_out_fd(size_t &i, size_t &initial_size, int fd, short revents, std::map<int, FdContext>::iterator it);

	/*HANDLE CLIENT FD*/
	bool handle_client_send(const int fd, size_t &initial_size);
	void advance_state(size_t pfd_i, Client &client);
	bool handle_client_recv(const int fd, const size_t index, size_t &initial_size);

	/* ADVANCE STATE */
	void state_sending_by_index(const size_t pfd_i, Client &client);
	void state_sending(Client &client); // rshin

	/* CGI */
	void handle_cgi_poll_fds(const size_t client_pfd, Client &client);
	void add_cgi_out_fd(Client &client);
	void add_cgi_in_fd(Client &client);
	void handle_cgi_error(Client *client, int fd, const size_t i, size_t &initial_size);
	bool read_from_cgi_fd(Client *client, int fd, size_t &i, size_t &initial_size);
	bool write_to_cgi_fd(Client *client, int fd, size_t &i, size_t &initial_size);
	bool try_finalize_cgi_response(Client &client);
	void remove_cgi_tracking(Client &client);

	/* HELPERS*/
	void fill_poll_listeners();
	void new_client(const int client_fd, Listener *listener);
	void cleanup_fd_at_index(const size_t i, size_t &initial_size, int fd);
	void state_sending_by_fd(int client_fd, Client &client);
	size_t find_pfd_index(int fd) const;
	void cleanup_fd_by_value(int fd, size_t &initial_size);
	void cleanup_client(int client_fd, size_t &initial_size);
	void clean_client_socket_only(Client &client, size_t &initial_size);
	void abort_client_running_cgi(Client &client, size_t &initial_size);
	bool client_has_running_cgi(Client &client) const;
	void print_fds();


	/* SIGNALS */
	void exit_cleanly(void);
	void handle_waiting_signals(void);
	void notify_child_event(pid_t pid, int status);

	/* TIMEOUT */
	void timeout_checker();
	bool is_client_timeout(Client &client, time_t time);
	void handle_timeout(Client &client);

	std::vector<pollfd> _pfds;
	std::vector<Listener *> _listeners;
	std::map<int, Client *> _clients;
	std::map<pid_t, Client *> _cgiClients;
	size_t _nbListeners;
	static const size_t _maxRequestSize = 34359738368; // ~4go + 8kb

	std::map<int, FdContext> _fdCtx;
};
