#include "client/Client.hpp"
#include "http/Response.hpp"
#include <string>

bool end_header(const std::string &str, size_t &headers_ends) 
{ 
	size_t pos = str.find("\r\n\r\n"); 
	if (pos == std::string::npos) 
		return false; 
	headers_ends = pos; 
	return true; 
}

bool end_body(const std::string &str, size_t content_len)
{
	if (content_len == 0)
		return true;
	if (str.size() < content_len)
		return false;
	return true;
}
void Client::select_location()
{
	if (!_selected_server)
		throw std::runtime_error("500");
	const LocationConfig *best = NULL;
	size_t best_len = 0;

	for (std::vector<LocationConfig>::const_iterator locIt =
			 _selected_server->locCfg.begin();
		 locIt != _selected_server->locCfg.end();
		 ++locIt)
	{
		const std::string &loc_path = locIt->path;

		if (_req.getPath().find(loc_path) == 0)
		{
			if (loc_path.length() > best_len)
			{
				best = &(*locIt);
				best_len = loc_path.length();
			}
		}
	}

	_selected_location = best;
	if (!_selected_location)
		throw std::runtime_error("500");
}

void Client::select_server()
{
	const std::vector<const Config *> &configs = _owner->getConfigs();

	std::string host = _req.getHeader("host");

	size_t colon = host.find(':');
	if (colon != std::string::npos)
		host = host.substr(0, colon);

	_selected_server = NULL;
	
	for (std::vector<const Config *>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
        if ((*it)->port != _owner->_port)
            continue;

        for (std::vector<std::string>::const_iterator sn = (*it)->serverNames.begin(); sn != (*it)->serverNames.end(); ++sn)
        {
            if (*sn == host)
            {
                _selected_server = (*it);
                return;
            }
        }
    }

    for (std::vector<const Config *>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
        if ((*it)->port != _owner->_port)
            continue;

        for (std::vector<std::string>::const_iterator sn = (*it)->serverNames.begin(); sn != (*it)->serverNames.end(); ++sn)
        {
            if (*sn == "default")
            {
                _selected_server = (*it);
                return;
            }
        }
    }

	for (std::vector<const Config *>::const_iterator it = configs.begin();
		 it != configs.end(); ++it)
	{
		if ((*it)->port == _owner->_port)
		{
			_selected_server = (*it);
			return;
		}
	}
	if (!_selected_server)
		throw std::runtime_error("No server found for port");
}

void Client::compare_ConfReq()
{

	if (!_selected_server)
		throw std::runtime_error("500");

	const std::vector<std::string> *allowed = NULL;

	if (_selected_location && !_selected_location->allowedMethods.empty())
		allowed = &_selected_location->allowedMethods;
	else
		allowed = &_selected_server->allowedMethods;

	bool method_allowed = false;
	for (std::vector<std::string>::const_iterator it = allowed->begin();
		 it != allowed->end(); ++it)
	{
		if (*it == _req.getMethod())
		{
			method_allowed = true;
			break;
		}
	}

	if (!method_allowed)
		throw std::runtime_error("405");

	ssize_t max_body = -1;

	if (_selected_location && _selected_location->clientMaxBodySize >= 0)
		max_body = _selected_location->clientMaxBodySize;
	else
		max_body = _selected_server->clientMaxBodySize;

	if (max_body >= 0 && (ssize_t)_req.getBody().size() > max_body)
		throw std::runtime_error("413");

	if ((size_t)_req.getBody().size() != _req.getContent_len() && !_req.getChunked())
		throw std::runtime_error("413");
}

void Client::updateLastActivity() { _last_activity = time(NULL); }
time_t Client::getLastActivity() const { return _last_activity; }
void Client::updateCgiLastActivity() { _cgi_last_activity = time(NULL); }
time_t Client::getCgiLastActivity() const { return _cgi_last_activity; }