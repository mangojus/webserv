#pragma once

#include "config/Config.hpp"

#include <vector>

class ConfigValidator
{
	public:
		ConfigValidator(std::vector<Config>& cfgs);
		~ConfigValidator();

		void validate() const;
		void validateLocations(const Config& cfg) const;
		void normalize();
		void normalizeLocations(Config& cfg);
		static std::vector<Config> buildDefaultConfig();
		static LocationConfig buildDefaultLocationConfig();

		void debug() const;

	private:
		std::vector<Config>& _cfgs;

		ConfigValidator();
		ConfigValidator(const ConfigValidator& copy);
		ConfigValidator& operator=(const ConfigValidator& other);
};
