#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <utils/Signals.hpp>

volatile sig_atomic_t g_sig_stop = 0;
volatile sig_atomic_t g_sig_child_dead = 0;

// SIGINT == ctrl + c
// SIGTERM == kill -15
// SIGCHLD == child process ended

void handle_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
		g_sig_stop = 1;
	else if (sig == SIGCHLD)
		g_sig_child_dead = 1;
}


void setup_signal(void)
{
	if (signal(SIGINT, handle_signal) == SIG_ERR)
	{
		throw std::runtime_error("signal SIGINT failed");
	}
	if (signal(SIGTERM, handle_signal) == SIG_ERR)
	{
		throw std::runtime_error("signal SIGTERM failed");
	}
	if (signal(SIGCHLD, handle_signal) == SIG_ERR)
	{
		throw std::runtime_error("signal SIGCHLD failed");
	}
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) // ignore SIGPIPE to avoid crashing when writing to a closed socket, we will handle the error in send() instead
	{
		throw std::runtime_error("signal SIGPIPE failed");
	}
}

// SIGCHLD == something happened to a child procress
// waitpid == check which child process ended and get its exit status, we use WNOHANG to avoid blocking if there is no child process that ended, we will call waitpid again when we receive the next SIGCHLD signal
// then we decide what to do