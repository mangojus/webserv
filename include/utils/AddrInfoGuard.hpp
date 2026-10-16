#pragma once

#include <netdb.h>
#include <stdexcept>

class AddrInfoGuard
{
	public:
		AddrInfoGuard() 
			: _ptr(NULL)
			{};
		~AddrInfoGuard();
		addrinfo **get_ptr();
		addrinfo *get() const;

	private:
		addrinfo *_ptr;
		AddrInfoGuard(const AddrInfoGuard& other);
		AddrInfoGuard& operator=(const AddrInfoGuard& other);
};