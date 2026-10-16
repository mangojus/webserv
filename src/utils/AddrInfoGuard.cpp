#include "utils/AddrInfoGuard.hpp"

AddrInfoGuard::~AddrInfoGuard()
{
	if (_ptr)
		freeaddrinfo(_ptr);
}

addrinfo **AddrInfoGuard::get_ptr()
{
	return &_ptr;
}

addrinfo *AddrInfoGuard::get() const
{
	return _ptr;
}