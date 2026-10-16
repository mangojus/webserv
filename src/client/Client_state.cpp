#include "client/Client.hpp"
#include "http/HttpParser.hpp"
#include "http/Response.hpp"

#include <string>
#include <cstdlib>

bool Client::try_parse_header()
{
	size_t tmp_header_size = 0;

	try
	{
		if (!end_header(_in_buffer, tmp_header_size)) // we have not received the full header yet
			return false;

		_headers_ends = tmp_header_size;
		_body_start_pos = tmp_header_size + 4;

		std::string raw_header = _in_buffer.substr(0, _body_start_pos);

		HttpParser pars;
		_req = pars.parse(raw_header);
		_header_parsed = true;

		_decoded_body.clear();
		_expected_body_len = 0;
		_chunk_pos = _body_start_pos;
		_chunk_remaining = 0;

		if (_req.getChunked())
		{
			_state = READING_CHUNK;
		}
		else if (_req.getHas_content_length())
		{
			_expected_body_len = _req.getContent_len();
			_state = READING_BODY;
		}
		else
		{
			_state = REQUEST_COMPLETE;
		}
		return true;
	}
	catch (const std::runtime_error &e)
	{
		_state = ERROR_STATE;
		_errorCode = e.what();
		return true;
	}
}

bool Client::try_read_body()
{
	size_t x = 0;

	try
	{
		if (_in_buffer.size() < _body_start_pos)
			return false;
		x = _in_buffer.size() - _body_start_pos;
		if (x < _expected_body_len) // there is still some body to read
			return false;
		else
		{
			_decoded_body.assign(_in_buffer, _body_start_pos, _expected_body_len);
			_req.setBody(_in_buffer.substr(_body_start_pos));
			setState(REQUEST_COMPLETE);
			return true;
		}
	}
	catch (const std::runtime_error &e)
	{
		_state = ERROR_STATE;
		_errorCode = e.what();
		return true;
	}
}

bool valid_hex(const char *str, long &val)
{
	errno = 0;
	char *endptr;
	long tmp;

	tmp = std::strtoul(str, &endptr, 16);
	if (errno != 0)
	{
		return false;
	}
	else if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n')
	{
		return false;
	}
	else if (tmp < 0)
	{
		return false;
	}

	val = tmp;
	return true;
}

bool Client::try_read_chunked()
{
	long chunk_size;
	size_t pos_end;
	size_t expected;
	size_t data_begin;
	size_t tmp_chunk_pos;

	std::string line;

	chunk_size = 0;
	try
	{
		while (1)
		{
			tmp_chunk_pos = _chunk_pos;
			pos_end = _in_buffer.find("\r\n", tmp_chunk_pos); // _chunk_pos = start of the body
			if (pos_end == std::string::npos)
				return false;

			line = _in_buffer.substr(tmp_chunk_pos, pos_end - tmp_chunk_pos);
			if (!valid_hex(line.c_str(), chunk_size))
			{
				throw std::runtime_error("400");
			}

			tmp_chunk_pos = pos_end + 2;
			if (chunk_size == 0)
			{
				if (_in_buffer.size() < tmp_chunk_pos + 2)
					return false;
				if (_in_buffer.compare(tmp_chunk_pos, 2, "\r\n") != 0)
				{
					throw std::runtime_error("400");
				}
				tmp_chunk_pos += 2;
				_req.setBody(_decoded_body);
				setState(REQUEST_COMPLETE);
				return true;
			}
			expected = static_cast<size_t>(chunk_size) + 2;
			if (_in_buffer.size() < tmp_chunk_pos + expected) // we have not received the full chunk yet
				return false;
			_decoded_body.append(_in_buffer, tmp_chunk_pos, chunk_size);
			data_begin = tmp_chunk_pos + chunk_size;
			if (_in_buffer.compare(data_begin, 2, "\r\n") != 0)
			{
				throw std::runtime_error("400");
			}
			tmp_chunk_pos = data_begin + 2;
			_chunk_pos = tmp_chunk_pos;
		}

	}
	catch (const std::runtime_error &e)
	{
		_state = ERROR_STATE;
		_errorCode = e.what();
		return true;
	}
}

void Client::handle_request()
{
	try
	{
		select_server();
		select_location();
	
		if (_selected_location->redir.hasRedir) {
			_state = BUILDING_STATIC_RESPONSE;
			return;
		}

		compare_ConfReq();
		std::string fs_path = resolve_path();
		fs_path = resolve_final_path(fs_path);

		_full_path = fs_path;
		_body = getRequest().getBody();
		_method = getRequest().getMethod();

		if (_isdir == false){
			if (hasCgiRequest())
			{
				start_cgi();
				return;
			}
		}
		_state = BUILDING_STATIC_RESPONSE;
	}
	catch (const std::runtime_error &e)
	{
		_state = ERROR_STATE;
		_errorCode = e.what();
	}
}

void Client::handle_response()
{
	try
	{
		Response res(_method, _full_path, _body, _req.getPath());
		if (_selected_location->redir.hasRedir)
		{
			_out_buffer = res.serveRedirect(_selected_location->redir.statusCode, _selected_location->redir.url);
		}
		else if (_method == "GET")
		{
			_out_buffer = res.serveGet();
		}
		else if (_method == "POST")
		{
			_out_buffer = res.servePost();
		}
		else if (_method == "DELETE")
		{
			_out_buffer = res.serveDelete();
		}
		else
		{
			_state = ERROR_STATE;
			_errorCode = "403";
			return;
		}
		_state = SENDING_RESPONSE;
	}
	catch (const std::exception &e)
	{
		_state = ERROR_STATE;
		_errorCode = e.what();
	}
}
void Client::handle_error()
{
	std::cerr << "Handle_error fd= " << _fd << " Error_code= " << _errorCode << std::endl;
	Response res(_errorCode);
	if (!_selected_server)
	{
		select_server();
	}
	_out_buffer = res.buildErrorResponse(_selected_server);
	_state = SENDING_RESPONSE;
}
