#pragma once

#include "config/Config.hpp"
#include "engine/Listener.hpp"
#include "engine/Engine.hpp"
#include "utils/UniquePair.hpp"

#include <string>
#include <vector>

class Booter
{
	public:
		Booter() : _cfgFileName("") {}
		~Booter() { destroy_listeners(); }

		void init(const std::string& configFile);
		void run();

		void debug() const;

	private:
		Booter(const Booter& other);
		Booter& operator=(const Booter& other);

		void build_listeners();
		void destroy_listeners();

		std::string _cfgFileName;
		std::vector<Config> _cfgs;

		std::map<UniquePair, Listener*> _listenerMap;
		std::vector<Listener*> _listeners;
};
