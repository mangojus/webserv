#include "config/ConfigValidator.hpp"

#include <string>
#include <iostream>

void printAllConfigs(const std::vector<Config>& cfgs);

void ConfigValidator::debug() const
{
	printAllConfigs(_cfgs);
}
