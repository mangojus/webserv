#include "engine/Booter.hpp"

#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigLexer.hpp"
#include "config/ConfigValidator.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

#include "engine/Listener.hpp"

void printConfigTokens(const std::vector<ConfigToken>& tokens);
void printAllConfigs(const std::vector<Config>& cfgs);

void Booter::debug() const
{
	std::cout << "==================== DEBUG:Booter ====================\n\n";
	printAllConfigs(_cfgs);
}
