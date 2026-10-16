#pragma once

#include "config/Config.hpp"
#include "config/ConfigToken.hpp"
#include "config/ConfigLexer.hpp"

#include <string>
#include <vector>

class ConfigParser
{
	public:
		ConfigParser() {}
		~ConfigParser() {}

		std::vector<Config> parse(const std::string& fileName);

		void debug() const;

	private:
		std::vector<ConfigToken> _tokens;

		ConfigParser(const ConfigParser& copy);
		ConfigParser& operator=(const ConfigParser& other);

		Config parseServer(std::vector<ConfigToken>::iterator& it);
		bool parseListen(Config& cfg, std::vector<ConfigToken>::iterator& it);
		bool parseServerName(Config& cfg, std::vector<ConfigToken>::iterator& it);
		bool parseLocation(Config& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseReturn(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseErrorPage(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseRoot(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseIndex(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseAutoIndex(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseAllowedMethods(T& cfg, std::vector<ConfigToken>::iterator& it);
		template<typename T>
			bool parseClientMaxBodySize(T& cfg, std::vector<ConfigToken>::iterator& it);
        bool parseCgi(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it);
        bool parseCgiPass(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it);
        bool parseCgiExtension(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it);
};
