#pragma once

# include <string>
# include <vector>
# include <map>
# include <exception>
# include <sys/types.h>

struct LocationFlags
{
	bool hasRoot;
	bool hasIndex;
	bool hasAutoIndex;
	bool hasRedir;
	bool hasAllowedMethods;
	bool hasClientMaxBodySize;
	bool hasCgi;
	bool hasCgiPass;
	bool hasCgiExtension;

	LocationFlags()
		: hasRoot(false)
		  , hasIndex(false)
		  , hasAutoIndex(false)
		  , hasRedir(false)
		  , hasAllowedMethods(false)
		  , hasClientMaxBodySize(false)
		  , hasCgi(false)
		  , hasCgiPass(false)
		  , hasCgiExtension(false)
	{}
};

struct ConfigFlags
{
	bool hasListen;
	bool hasBacklog;
	bool hasServerName;
	bool hasRoot;
	bool hasIndex;
	bool hasAutoIndex;
	bool hasRedir;
	bool hasAllowedMethods;
	bool hasClientMaxBodySize;
	bool hasErrorPage;
	bool hasLocation;

	ConfigFlags()
		: hasListen(false)
		  , hasBacklog(false)
		  , hasServerName(false)
		  , hasRoot(false)
		  ,	hasIndex(false)
		  , hasAutoIndex(false)
		  , hasRedir(false)
		  , hasAllowedMethods(false)
		  , hasClientMaxBodySize(false)
		  , hasErrorPage(false)
		  , hasLocation(false)
	{}
};

struct Redirect
{
	std::string statusCode;
	std::string url;
	bool hasRedir;

	Redirect()
		: hasRedir(false)
	{}
};

struct LocationConfig
{
	std::string path;
	std::string root;
	std::string index;
	bool autoIndex;
	Redirect redir;
	std::vector<std::string> allowedMethods;
	ssize_t clientMaxBodySize;
	bool hasCgi;
	std::string cgiPass;
	std::string cgiExtension;

	LocationFlags flag;

	LocationConfig()
		: autoIndex(-1)
		  , clientMaxBodySize(-1)
		  , hasCgi(false)
	{}
};

struct Config
{
	std::string host;
	int port;
	int backlog;
	std::vector<std::string> serverNames;
	std::string root;
	std::string index;
	bool autoIndex;
	Redirect redir;
	std::vector<std::string> allowedMethods;
	ssize_t clientMaxBodySize;
	std::map<std::string, std::string> errorPages;
	bool hasErrorPages;
	std::vector<LocationConfig> locCfg;

	ConfigFlags flag;

	Config()
		: port(-1)
		  , backlog(-1)
		  , autoIndex(-1)
		  , clientMaxBodySize(-1)
		  , hasErrorPages(false)
	{}
};

class ConfigError: public std::exception
{
	public:
		ConfigError(const std::string& msg) : _message(msg) {}
		ConfigError(const std::string& msg, const std::string& line) : _message(msg + ", at line: " + line) {}
		virtual ~ConfigError() throw() {}
		virtual const char* what() const throw() { return _message.c_str(); }
	private:
		std::string _message;
};
