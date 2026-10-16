#include "config/ConfigParser.hpp"

#include "config/ConfigValidator.hpp"
#include "config/Config.hpp"
#include "config/ConfigToken.hpp"
#include "utils/StringUtils.hpp"

#include <string>
#include <fstream>
#include <exception>
#include <cerrno>
#include <cstdlib>
#include <sstream>

static bool isValidIPv4(const std::string& ip)
{
    std::istringstream ss(ip);
    std::string part;
    int count = 0;

    while (std::getline(ss, part, '.'))
    {
        if (part.empty())
            return false;

        if (++count > 4)
            return false;

        // reject leading zeros
        if (part.size() > 1 && part[0] == '0')
            return false;

        for (size_t i = 0; i < part.size(); i++)
            if (!isdigit(part[i]))
                return false;

        char* end = NULL;
        long val = std::strtol(part.c_str(), &end, 10);

        if (*end != '\0' || val < 0 || val > 255)
            return false;
    }
    return count == 4;
}

bool ConfigParser::parseListen(Config& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "listen") {
		return false;
	}

	if (cfg.flag.hasListen == true)
		throw ConfigError("Duplicate listen directive not allowed", it->line);
	cfg.flag.hasListen = true;

	++it;

	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'listen'", it->line);
	}

	std::string host;
	long port;
	char* endptr = NULL;

	size_t pos = it->value.find(':');
	if (pos != std::string::npos) {
		host = it->value.substr(0, pos);
		std::string portStr = it->value.substr(pos + 1);
		if (portStr.empty()) {
			throw ConfigError("Port missing after ':'", it->line);
		}
		errno = 0;
		port = std::strtol(portStr.c_str(), &endptr, 10);
	}
	else if (utils::isNumber(it->value)) {
		host = "0.0.0.0";
		errno = 0;
		port = std::strtol(it->value.c_str(), &endptr, 10);
	}
	else {
		throw ConfigError("Invalid listen value: " + it->value, it->line);
	}
	if (*endptr != '\0') {
		throw ConfigError("Port contains invalid characters", it->line);
	}
	if (errno == ERANGE || port < 1 || port > 65535) {
		throw ConfigError("Port out of range", it->line);
	}
	if (host != "0.0.0.0" && host != "localhost")
	{
		if (!isValidIPv4(host))
			throw ConfigError("Invalid host in listen directive: " + host, it->line);
	}
	++it;
	while (it->type == ID) {
		size_t pos = it->value.find('=');
		if (pos != std::string::npos) {
			std::string key = it->value.substr(0, pos);
			if (key != "backlog") {
				throw ConfigError("Unknown option after listen directive", it->line);
			}
			std::string value = it->value.substr(pos + 1);
			if (value.empty()) {
				throw ConfigError("Value missing after '='", it->line);
			}
			endptr = NULL;
			errno = 0;
			long val = std::strtol(value.c_str(), &endptr, 10);
			if (*endptr != '\0') {
				throw ConfigError("Invalid value for backlog", it->line);
			}
			if (errno == ERANGE || val <= 0 || val > 4096) {
				throw ConfigError("Invalid value range for backlog", it->line);
			}
			cfg.backlog = static_cast<int>(val);
			++it;
		}
		else {
			throw ConfigError("Unknown option after listen directive", it->line);
		}
	}
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after listen directive", it->line);
	}
	++it;
	cfg.host = host;
	cfg.port = static_cast<int>(port);
	return true;
}

bool ConfigParser::parseServerName(Config& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "server_name") {
		return false;
	}

	if (cfg.flag.hasServerName == true)
		throw ConfigError("Duplicate server_name directive not allowed", it->line);
	cfg.flag.hasServerName = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'server_name'", it->line);
	}
	std::vector<std::string> serverNames;
	while (it->type == ID) {
		serverNames.push_back(it->value);
		++it;
	}
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after server_name directive", it->line);
	}
	++it;
	cfg.serverNames = serverNames;
	return true;
}

template<typename T>
bool ConfigParser::parseRoot(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "root") {
		return false;
	}

	if (cfg.flag.hasRoot == true)
		throw ConfigError("Duplicate root directive not allowed", it->line);
	cfg.flag.hasRoot = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'root'", it->line);
	}
	std::string path = it->value;
	if (!path.empty()) {
		if (path[0] == '/') {
			throw ConfigError("root: Path must not start with '/', cannot be absolute", it->line);
		}
		if (path.substr(0, 2) == "./") {
			throw ConfigError("root: Path must not start with './', cannot be explicitely relative", it->line);	
		}
		std::istringstream iss(path);
		std::string segment;
		while (std::getline(iss, segment, '/')) {
			if (segment == "..") {
				throw ConfigError("root: Path must not contain '..' segment", it->line);
			}
		}
	}
	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after root directive", it->line);
	}
	++it;
	cfg.root = path;
	return true;
}

template<typename T>
bool ConfigParser::parseIndex(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "index") {
		return false;
	}

	if (cfg.flag.hasIndex == true)
		throw ConfigError("Duplicate index directive not allowed", it->line);
	cfg.flag.hasIndex = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'index'", it->line);
	}
	std::string index = it->value;
	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after index directive", it->line);
	}
	cfg.index = index;
	++it;
	return true;
}

template<typename T>
bool ConfigParser::parseAutoIndex(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "autoindex") {
		return false;
	}

	if (cfg.flag.hasAutoIndex == true)
		throw ConfigError("Duplicate autoindex directive not allowed", it->line);
	cfg.flag.hasAutoIndex = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'autoindex'", it->line);
	}
	if (it->value == "on") {
		cfg.autoIndex = 1;
	}
	else if (it->value == "off") {
		cfg.autoIndex = 0;
	}
	else {
		throw ConfigError("Unknown argument for 'autoindex' directive", it->line);
	}
	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after autoindex directive", it->line);
	}
	++it;
	return true;
}

template<typename T>
bool ConfigParser::parseReturn(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "return") {
		return false;
	}

	if (cfg.flag.hasRedir == true)
		throw ConfigError("Duplicate return directive not allowed", it->line);
	cfg.flag.hasRedir = true;
	cfg.redir.hasRedir = true;

	++it;

	if (it->type != ID || !utils::isNumber(it->value)) {
		throw ConfigError("Invalid status code in 'return' directive", it->line);
	}

	std::string statusCode = it->value;
	if (statusCode.size() != 3 || statusCode[0] != '3') {
		throw ConfigError("Status code out of range: " + statusCode, it->line);
	}
	cfg.redir.statusCode = statusCode;
	++it;

	std::string url = it->value;
	if (it->type == ID) {
		if (url.empty()) {
			throw ConfigError("Missing URL in 'return' directive", it->line);
		}
		if (url[0] != '/' && url.find("http://") != 0 && url.find("https://") != 0) {
			throw ConfigError("URL must be a path or full URL", it->line);
		}
		cfg.redir.url = url;
		++it;
	}

	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after return directive", it->line);
	}
	++it;
	return true;
}

template<typename T>
bool ConfigParser::parseAllowedMethods(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "allowed_methods") {
		return false;
	}

	if (cfg.flag.hasAllowedMethods == true)
		throw ConfigError("Duplicate allowed_methods directive not allowed", it->line);
	cfg.flag.hasAllowedMethods = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'allowed_methods'", it->line);
	}
	std::vector<std::string> allowedMethods;
	while (it->type == ID) {
		if (it->value == "GET" || it->value == "POST" || it->value == "DELETE") {
			allowedMethods.push_back(it->value);
			++it;
		}
		else {
			throw ConfigError("Invalid method: " + it->value, it->line); 
		}
	}
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after allowed_methods directive", it->line);
	}
	++it;
	cfg.allowedMethods = allowedMethods;
	return true;
}

template<typename T>
bool ConfigParser::parseClientMaxBodySize(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "client_max_body_size") {
		return false;
	}

	if (cfg.flag.hasClientMaxBodySize == true)
		throw ConfigError("Duplicate client_max_body_size directive not allowed", it->line);
	cfg.flag.hasClientMaxBodySize = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'client_max_body_size'", it->line);
	}
	char *endptr = NULL;
	errno = 0;
	long clientMaxBodySize = std::strtol(it->value.c_str(), &endptr, 10);
	if (*endptr != '\0')
		throw ConfigError("ClientMaxBodySize value contains invalid characters", it->line);
	if (errno == ERANGE || clientMaxBodySize < 0) {
		throw ConfigError("ClientMaxBodySize value out of range", it->line);
	}
	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after client_max_body_size directive", it->line);
	}
	++it;
	cfg.clientMaxBodySize = static_cast<ssize_t>(clientMaxBodySize);
	return true;
}

template<typename T>
bool ConfigParser::parseErrorPage(T& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "error_page") {
		return false;
	}
	cfg.flag.hasErrorPage = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'error_page'", it->line);
	}

	std::vector<std::string> statusCodes;

	while (it->type == ID && utils::isNumber(it->value)) {
		if (it->value.size() != 3 || it->value[0] < '4' || it->value[0] > '5') {
			throw ConfigError("Status code out of range: " + it->value, it->line);
		}
		statusCodes.push_back(it->value);
		++it;
	}

	if (it->type != ID) {
		throw ConfigError("Missing path in error_page directive", it->line);
	}

	std::string path = it->value;
	if (!path.empty()) {
		if (path[0] == '/') {
			throw ConfigError("error_page: Path must not start with '/', cannot be absolute", it->line);
		}
		if (path.substr(0, 2) == "./") {
			throw ConfigError("error_page: Path must not start with './', cannot be explicitely relative", it->line);	
		}
		std::istringstream iss(path);
		std::string segment;
		while (std::getline(iss, segment, '/')) {
			if (segment == "..") {
				throw ConfigError("error_page: Path must not contain '..' segment", it->line);
			}
		}
	}
	++it;

	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after error_page directive", it->line);
	}
	++it;

	for (size_t i = 0; i < statusCodes.size(); ++i) {
		if (cfg.errorPages.find(statusCodes[i]) != cfg.errorPages.end()) {
			throw ConfigError("Duplicate status code for error_page directive: " + statusCodes[i], it->line);
		}
		cfg.errorPages[statusCodes[i]] = path;
	}
	return true;
}

bool ConfigParser::parseCgi(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "cgi") {
		return false;
	}

	if (cfg.flag.hasCgi == true)
		throw ConfigError("Duplicate cgi directive not allowed", it->line);
	cfg.flag.hasCgi = true;

	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after cgi directive", it->line);
	}
	++it;
	cfg.hasCgi = true;
	return true;
}

bool ConfigParser::parseCgiPass(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "cgi_pass") {
		return false;
	}

	if (cfg.flag.hasCgiPass == true)
		throw ConfigError("Duplicate cgi_pass directive not allowed", it->line);
	cfg.flag.hasCgiPass = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'cgi_pass'", it->line);
	}
	std::string cgiPass = it->value;
	if (cgiPass[0] != '/')
		throw ConfigError("cgi_pass must be an absolute path", it->line);
	++it; 
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after cgi_pass directive", it->line);
	}
	++it;
	cfg.cgiPass = cgiPass;
	return true;
}

bool ConfigParser::parseCgiExtension(LocationConfig& cfg, std::vector<ConfigToken>::iterator& it)
{
	if (it->type != ID || it->value != "cgi_extension") {
		return false;
	}

	if (cfg.flag.hasCgiExtension == true)
		throw ConfigError("Duplicate cgi_extension directive not allowed", it->line);
	cfg.flag.hasCgiExtension = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'cgi_extension'", it->line);
	}
	std::string cgiExtension = it->value;
	++it;
	if (it->type != SEMICOLON) {
		throw ConfigError("Missing ';' after cgi_extension directive", it->line);
	}
	++it;
	cfg.cgiExtension = cgiExtension;
	return true;
}

bool ConfigParser::parseLocation(Config& cfg, std::vector<ConfigToken>::iterator& it)
{
	LocationConfig locCfg;

	if (it->type != ID || it->value != "location") {
		return false;
	}
	cfg.flag.hasLocation = true;

	++it;
	if (it->type != ID) {
		throw ConfigError("Unexpected end after 'location'", it->line);
	}
	locCfg.path = it->value;
	if (locCfg.path.empty())
		throw ConfigError("Empty location path", it->line);
	if (locCfg.path[0] != '/') {
		throw ConfigError("Location path must start with '/'", it->line);
	}
	std::istringstream iss(locCfg.path);
	std::string segment;
	while (std::getline(iss, segment, '/')) {
		if (segment == "..") {
			throw ConfigError("Location path must not contain '..' segment", it->line);
		}
	}
	for (std::vector<LocationConfig>::iterator locIt = cfg.locCfg.begin(); locIt != cfg.locCfg.end(); ++locIt) {
		if (locCfg.path == locIt->path) {
			throw ConfigError("Duplicate location directive not allowed", it->line);
		}
	}
	++it;
	if (it->type != LBRACE)
		throw ConfigError("Expected '{' before location block", it->line);
	++it;
	while (it->type != END && it->type != RBRACE) {
		if (it->type == ID && it->value == "location") {
			throw ConfigError("Nested locations are not allowed", it->line);
		}
		if (parseRoot(locCfg, it)) continue;
		if (parseIndex(locCfg, it)) continue;
		if (parseAutoIndex(locCfg, it)) continue;
		if (parseAllowedMethods(locCfg, it)) continue;
		if (parseClientMaxBodySize(locCfg, it)) continue;
		if (parseReturn(locCfg, it)) continue;
		if (parseCgi(locCfg, it)) continue;
		if (parseCgiPass(locCfg, it)) continue;
		if (parseCgiExtension(locCfg, it)) continue;
		throw ConfigError("Unexpected token inside location block: " + it->value, it->line);
	}

	if (it->type != RBRACE)
		throw ConfigError("Expected '}' after location block", it->line);
	++it;
	cfg.locCfg.push_back(locCfg);
	return true;
}

Config ConfigParser::parseServer(std::vector<ConfigToken>::iterator& it)
{
	Config servCfg;

	if (it->type != LBRACE)
		throw ConfigError("Expected '{' before server block", it->line);
	++it;

	while (it->type != END && it->type != RBRACE) {
		if (parseListen(servCfg, it)) { continue; }
		if (parseRoot(servCfg, it)) { continue; }
		if (parseIndex(servCfg, it)) { continue; }
		if (parseServerName(servCfg, it)) continue;
		if (parseAllowedMethods(servCfg, it)) continue;
		if (parseClientMaxBodySize(servCfg, it)) continue;
		if (parseReturn(servCfg, it)) continue;
		if (parseLocation(servCfg, it)) continue;
		if (parseAutoIndex(servCfg, it)) { continue; }
		if (parseErrorPage(servCfg, it)) continue;
		throw ConfigError("Unexpected token inside server block: " + it->value, it->line);
	}

	if (it->type != RBRACE)
		throw ConfigError("Expected '}' after server block", it->line);
	++it;
	return servCfg;
}

std::vector<Config> ConfigParser::parse(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (!file.is_open()) {
		throw ConfigError("Cannot open file" + fileName);
	}
	ConfigLexer lexer;
	std::vector<ConfigToken> tokens = lexer.tokenize(file);

	std::vector<Config> cfgs;

	std::vector<ConfigToken>::iterator it = tokens.begin();
	if (it->type == END) {
		throw ConfigError("Emtpy conf file");
	}
	while (it->type != END)
	{
		if (it->type == ID && it->value == "server") {
			++it;
			cfgs.push_back(parseServer(it));
		}
		else {
			throw ConfigError("Unexpected token outside server block: " + it->value, it->line);
		}
	}
	ConfigValidator validator(cfgs);
	validator.normalize();
	validator.validate();
	return cfgs;
}
