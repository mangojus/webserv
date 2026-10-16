#include "engine/Booter.hpp"
#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigValidator.hpp"
#include "utils/UniquePair.hpp"

#include <string>
#include <iostream>

#include "engine/Listener.hpp"

typedef std::map<UniquePair, Listener *> ListenerMap;

void Booter::init(const std::string& configFile)
{
	ConfigParser parser;

	if (configFile.empty()) {
		_cfgs = ConfigValidator::buildDefaultConfig();
	}
	else {
		_cfgs = parser.parse(configFile);
		_cfgFileName = configFile;
	}

	build_listeners();
	std::cout << "Server is initialized !\n";
}

void Booter::run()
{
	Engine engine(_listeners);

	std::cout << "Server is running...\n";
	engine.run();
}

void Booter::build_listeners()
{
	size_t i = 0;
	while (i < _cfgs.size())
	{
		std::string host = _cfgs[i].host;
		int port = _cfgs[i].port;

		if (host.empty())
			host = "0.0.0.0";

		UniquePair upa(host, port);

		ListenerMap::iterator it = _listenerMap.find(upa);

		if (it == _listenerMap.end())
		{
			Listener *listener = new Listener(host, port);
			_listenerMap.insert(std::make_pair(upa, listener));
			_listeners.push_back(listener);
			listener->addConfig(&_cfgs[i]);
		}
		else
		{
			it->second->addConfig(&_cfgs[i]);
		}
		++i;
	}
	i = 0;
	while (i < _listeners.size())
	{
		_listeners[i]->open_socket();
		++i;
	}
}

void Booter::destroy_listeners()
{
	std::map<UniquePair, Listener*>::iterator it = _listenerMap.begin();
	while(it != _listenerMap.end())
	{
		delete it->second;
		++it;
	}
	_listenerMap.clear();
	_listeners.clear();
}

