#include "client/Client.hpp"

#include <string>
#include <iostream>

void Client::debug() const
{
	std::cout << "==================== DEBUG:Client ====================\n\n";
	std::cout << "Client fd: " << _fd << "\n";
	std::cout << "Client state: " << _state << "\n";
	std::cout << "Client owner: " << _owner << "\n";
	std::cout << "End of header: " << _headers_ends << "\n";
	std::cout << "Start of body: " << _body_start_pos << "\n";
	std::cout << "Expected body length: " << _expected_body_len << "\n";
	std::cout << "Chunk position: " << _chunk_pos << "\n";
	std::cout << "Is header parsed: " << _header_parsed << "\n";
	std::cout << "Selected server adress: " << _selected_server << "\n";
	std::cout << "Selected location adress: " << _selected_location << "\n";
	std::cout << std::endl;
	if (_in_buffer.empty()) {
		std::cout << "Client in buffer/REQUEST: *EMPTY*\n";
	}
	else {
		std::cout << "Client in buffer/REQUEST:\n\n" << "********************BOF\n" << _in_buffer << "EOF********************\n\n";
	}
	if (_out_buffer.empty()) {
		std::cout << "Client out buffer/RESPONSE: *EMPTY*\n";
	}
	else {
		std::cout << "Client out buffer/RESPONSE:\n\n" << "********************BOF\n" << _out_buffer << "EOF********************\n\n";
	}
	std::cout << std::endl;
}
