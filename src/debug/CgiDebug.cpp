#include "http/Cgi.hpp"

#include <string>
#include <iostream>

void Cgi::debug() const
{
	std::cout << "==================== DEBUG:Cgi ====================\n\n";
	std::cout << "Cgi state: " << getCgiState() << "\n";
	std::cout << "Cgi pid: " << getPid() << "\n";
	std::cout << "Cgi in buffer:\n********************BOF\n" << getInBuf() << "\nEOF********************\n";
	std::cout << "Cgi out buffer:\n********************BOF\n" << getOutBuf() << "\nEOF********************\n";
	std::cout << std::endl;
	//  std::cout << "Cgi envp: " << 
}
