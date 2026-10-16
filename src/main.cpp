#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>

#include "engine/Booter.hpp"

int main(int argc, char **argv)
{
	if (argc > 2) {
		std::cerr << "Too many args\n";
		return EXIT_FAILURE;
	}
	try {
		Booter app;
		std::string file;
		if (argc == 2) {
			file = argv[1];
		}
		app.init(file);
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
