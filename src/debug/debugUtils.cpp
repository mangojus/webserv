#include "config/Config.hpp"
#include "config/ConfigLexer.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigValidator.hpp"
#include "http/Request.hpp"
#include "http/Cgi.hpp"
#include "client/Client.hpp"
#include "engine/Listener.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <map>

void printConfigTokens(const std::vector<ConfigToken>& tokens)
{
	for (std::vector<ConfigToken>::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
	{
		std::cout << "[";
		switch (it->type)
		{
			case ID:
				{
					std::cout << "ID" << ":";
					break;
				}
			case LBRACE:
				{
					std::cout << "LBRACE" << ":";
					break;
				}
			case RBRACE:
				{
					std::cout << "RBRACE" << ":";
					break;
				}
			case SEMICOLON:
				{
					std::cout << "SEMICOLON" << ":";
					break;
				}
			case END:
				{
					std::cout << "END" << ":";
					break;
				}
		}
		std::cout << it->value << "] ";
	}
	std::cout << "\n" << std::endl;
}

void printLocation(const LocationConfig& loc)
{
	std::cout << "----- Location -----\n\n";
	std::cout << "Path: " << loc.path << "\n";
	std::cout << "Root: " << loc.root << "\n";
	std::cout << "Index: " << loc.index << "\n";
	std::cout << "Redirection:\n	hasredir: " << loc.redir.hasRedir << "\n"
	<< "	statuscode: " << loc.redir.statusCode << "\n"
	<< "	url: "<< loc.redir.url << "\n";

	std::cout << "Autoindex: " << loc.autoIndex << "\n";
	std::cout << "Allowed Methods: ";
	for (size_t i = 0; i < loc.allowedMethods.size(); ++i)
	{
		std::cout << loc.allowedMethods[i] << " ";
	}
	std::cout << "\n";

	std::cout << "Client Max Body Size: " << loc.clientMaxBodySize << "\n";
	std::cout << "Has Cgi: " << loc.hasCgi << "\n";
	std::cout << "Cgi Pass: " << loc.cgiPass << "\n";
	std::cout << "Cgi Extension: " << loc.cgiExtension << "\n";
	std::cout << std::endl;
}

void printAllLocations(const Config& cfg)
{
	for (size_t i = 0; i < cfg.locCfg.size(); ++i) {
		printLocation(cfg.locCfg[i]);
	}
}

void printConfig(const Config& cfg)
{
	std::cout << "----- Config -----\n\n";
	std::cout << "Host: " << cfg.host << "\n";
	std::cout << "Port: " << cfg.port << "\n";
	std::cout << "Backlog: " << cfg.backlog << "\n";
	std::cout << "Redirection:\n	hasredir: " << cfg.redir.hasRedir << "\n"
	<< "	statuscode: " << cfg.redir.statusCode << "\n"
	<< "	url: "<< cfg.redir.url << "\n";

	std::cout << "Server Names: ";
	for (size_t i = 0; i < cfg.serverNames.size(); ++i)
	{
		std::cout << cfg.serverNames[i] << " ";
	}
	std::cout << "\n";

	std::cout << "Root: " << cfg.root << "\n";
	std::cout << "Index: " << cfg.index << "\n";

	std::cout << "AutoIndex: " << cfg.autoIndex << "\n";

	std::cout << "Allowed Methods: ";
	for (size_t i = 0; i < cfg.allowedMethods.size(); ++i)
	{
		std::cout << cfg.allowedMethods[i] << " ";
	}
	std::cout << "\n";

	std::cout << "Client Max Body Size: " << cfg.clientMaxBodySize << "\n";

	std::map<std::string, std::string>::const_iterator err_it = cfg.errorPages.begin();
	while (err_it != cfg.errorPages.end())
	{
		std::cout << "Error Pages: " << err_it->first << "|" << err_it->second << "\n";
		++err_it;
	}
	std::cout << std::endl;
}

void printAllConfigs(const std::vector<Config>& cfgs)
{
	for (std::vector<Config>::const_iterator it = cfgs.begin(); it != cfgs.end(); ++it) {
		printConfig(*it);
		printAllLocations(*it);
	}
}

void printRequest(const Request& req)
{
	std::cout << "----- Request -----\n\n";
	std::cout << "Method: " << req.getMethod() << "\n";
	std::cout << "Path: " << req.getPath() << "\n";
	std::cout << "Query: " << req.getQuery() << "\n";
	std::cout << "Version: " << req.getVersion() << "\n";
	std::cout << "Body: " << req.getBody() << "\n";
	std::cout << "Has content length: " << req.getHas_content_length() << "\n";
	std::cout << "Content length: " << req.getContent_len() << "\n";
	std::cout << "Has chunked: " << req.getChunked() << "\n";
	std::cout << "Headers:\n" << req.getHeaders() << "\n";
	std::cout << std::endl;
}

void printListener(const Listener& listener)
{
	std::cout << "----- Listener -----\n\n";
	std::cout << "Listener host: " << listener._host << "\n";
	std::cout << "Listener port: " << listener._port << "\n";
	std::cout << "Server fd: " << listener.getFd() << "\n";
	//	std::cout << "Backlog: " << listener.getBacklog() << "\n";
	std::cout << std::endl;
}

void printAllListeners(const std::vector<Listener>& listeners)
{
	for (std::vector<Listener>::const_iterator it = listeners.begin(); it != listeners.end(); ++it) {
		printListener(*it);
	}
}
/*
void printClient(const Client& client)
{
	std::cout << "----- Client -----\n\n";
	std::cout << "Client fd: " << client._fd << "\n";
	std::cout << "Client state: " << client._state << "\n";
	std::cout << "Client owner: " << client._owner << "\n";
	std::cout << "End of header: " << client._headers_ends << "\n";
	std::cout << "Start of body: " << client._body_start_pos << "\n";
	std::cout << "Expected body length: " << client._expected_body_len << "\n";
	std::cout << "Chunk position: " << client._chunk_pos << "\n";
	std::cout << "Is header parsed: " << client._header_parsed << "\n";
	std::cout << "Selected server adress: " << client._selected_server << "\n";
	std::cout << "Selected location adress: " << client._selected_location << "\n";
	std::cout << std::endl;
	if (client._in_buffer.empty()) {
		std::cout << "Client in buffer/REQUEST: *EMPTY*\n";
	}
	else {
		std::cout << "Client in buffer/REQUEST:\n\n" << "********************BOF\n" << client._in_buffer << "EOF********************\n\n";
	}
	if (client._out_buffer.empty()) {
		std::cout << "Client out buffer/RESPONSE: *EMPTY*\n";
	}
	else {
		std::cout << "Client out buffer/RESPONSE:\n\n" << "********************BOF\n" << client._out_buffer << "EOF********************\n\n";
	}
	std::cout << std::endl;
}*/

void printCgi(const Cgi& cgi)
{
	std::cout << "----- Cgi -----\n\n";
	std::cout << "Cgi state: " << cgi.getCgiState() << "\n";
	std::cout << "Cgi pid: " << cgi.getPid() << "\n";
	std::cout << "Cgi in buffer: " << cgi.getInBuf() << "\n";
	std::cout << "Cgi out buffer: " << cgi.getOutBuf() << "\n";
	//	std::cout << "Cgi envp: " << cgi.
	std::cout << std::endl;
}
