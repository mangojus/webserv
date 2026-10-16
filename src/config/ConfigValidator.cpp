#include "config/ConfigValidator.hpp"

#include "config/Config.hpp"
#include "utils/StringUtils.hpp"
#include <sstream>

ConfigValidator::ConfigValidator(std::vector<Config>& cfgs) : _cfgs(cfgs) {}
ConfigValidator::~ConfigValidator() {}

LocationConfig ConfigValidator::buildDefaultLocationConfig()
{
	LocationConfig loc;
	loc.path = "/";
	loc.root = "www/html";
	loc.index = "index.html";
	loc.autoIndex = false;
	loc.allowedMethods.push_back("GET");
	loc.clientMaxBodySize = 1048576;
	loc.hasCgi = false;
	loc.redir.hasRedir = false;
	loc.cgiPass = "";

	return loc;
}

std::vector<Config> ConfigValidator::buildDefaultConfig()
{
	std::vector<Config> cfgs;

	LocationConfig loc = buildDefaultLocationConfig();

	Config cfg;
	cfg.host = "localhost";
	cfg.port = 8080;
	cfg.backlog = 128;
	cfg.serverNames.push_back("default");
	cfg.root = loc.root;
	cfg.index = loc.index;
	cfg.autoIndex = false;
	cfg.redir.hasRedir = false;
	cfg.allowedMethods = loc.allowedMethods;
	cfg.clientMaxBodySize = loc.clientMaxBodySize;
	cfg.hasErrorPages = false;

	cfg.locCfg.push_back(loc);
	cfgs.push_back(cfg);
	return cfgs;
}

void ConfigValidator::validateLocations(const Config& cfg) const
{
	std::vector<LocationConfig>::const_iterator it = cfg.locCfg.begin();
	while (it != cfg.locCfg.end()) {
		if (it->flag.hasCgi || it->flag.hasCgiPass || it->flag.hasCgiExtension) {
			if (!it->flag.hasCgi || !it->flag.hasCgiPass || !it->flag.hasCgiExtension) {
				std::string missing;
				if (!it->flag.hasCgi) {
					missing += "'cgi' ";
				}
				if (!it->flag.hasCgiPass) {
					missing += "'cgi_pass' ";
				}
				if (!it->flag.hasCgiExtension) {
					missing += "'cgi_extension'";
				}
				throw ConfigError("Location '" + it->path + "': CGI config incomplete, missing " + missing);
			}
		}
		++it;
	}
}

void ConfigValidator::normalizeLocations(Config& cfg)
{
	std::vector<LocationConfig>::iterator it = cfg.locCfg.begin();
	while (it != cfg.locCfg.end()) {
		if (!it->flag.hasRoot) {
			it->root = cfg.root;
		}
		if (!it->flag.hasIndex) {
			it->index = cfg.index;
		}
		if (!it->flag.hasAutoIndex) {
			it->autoIndex = cfg.autoIndex;
		}
		if (!it->flag.hasRedir) {
			it->redir = cfg.redir;
		}
		if (!it->flag.hasAllowedMethods) {
			it->allowedMethods = cfg.allowedMethods;
		}
		if (!it->flag.hasClientMaxBodySize) {
			it->clientMaxBodySize = cfg.clientMaxBodySize;
		}
		++it;
	}
}

void ConfigValidator::validate() const
{
	std::vector<Config>::const_iterator it = _cfgs.begin();

	std::vector<std::string> seen;

	while (it != _cfgs.end())
	{
		if (!it->flag.hasRoot)
			throw ConfigError("Root directive cannot be missing");
		if (!it->flag.hasListen)
			throw ConfigError("Listen directive cannot be missing");
		if (it->port < 1 || it->port > 65535) {
			throw ConfigError("Port out of range: " + utils::intToStr(it->port));
		}
		for (size_t i = 0; i < it->allowedMethods.size(); ++i) {
			const std::string& method = it->allowedMethods[i];
			if (method != "GET" && method != "POST" && method != "DELETE")
				throw ConfigError("Invalid method: " + method);
		}

		validateLocations(*it);

		for (size_t i = 0; i < it->serverNames.size(); i++)
		{
			const std::string& name = it->serverNames[i];

			for (size_t j = 0; j < seen.size(); j++)
			{
				if (seen[j] == name)
					throw ConfigError("Duplicate server_name: " + name);
			}
			seen.push_back(name);
		}
		++it;
	}
}

void ConfigValidator::normalize()
{
	for (std::vector<Config>::iterator it = _cfgs.begin(); it != _cfgs.end(); ++it) {
		if (!it->flag.hasListen) {
			it->host = "0.0.0.0";
			it->port = 8080;
		}
		if (!it->flag.hasRoot) {
			it->root = "www/html";
		}
		if (!it->flag.hasBacklog) {
			it->backlog = 128;
		}
		if (!it->redir.hasRedir) {
			it->redir.hasRedir = false;
		}
		if (!it->flag.hasIndex) {
			it->index = "index.html";
		}
		if (!it->flag.hasAutoIndex) {
			it->autoIndex = false;
		}
		if (!it->flag.hasAllowedMethods) {
			it->allowedMethods.push_back("GET");
			it->allowedMethods.push_back("POST");
			it->allowedMethods.push_back("DELETE");
		}
		if (!it->flag.hasClientMaxBodySize) {
			it->clientMaxBodySize = 1048576;
		}
		if (!it->flag.hasErrorPage) {
			it->hasErrorPages = false;
		}
		if (!it->flag.hasLocation) {
			it->locCfg.push_back(buildDefaultLocationConfig());
		}
		normalizeLocations(*it);
	}
}
