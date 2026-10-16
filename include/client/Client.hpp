#pragma once

#include <string>
#include "config/Config.hpp"
#include "engine/Listener.hpp"
#include "http/Request.hpp"
#include "http/Cgi.hpp"

class Listener;
class Request;

enum State
{
	READING_HEADER = 0,			// waiting for \r\n\r\n
	READING_BODY = 1,			// waiting for body of size Content-Length
	READING_CHUNK = 2,			// waiting for chunked body, not implemented yet
	REQUEST_COMPLETE = 3,		// request fully received, we will decide what to do next (CGI or static)
	CGI_WRITING = 4,			// waiting for stdin CGI to be ready for writing (POST body)
	CGI_READING = 5,   			// waiting for stdout CGI to be ready for reading (CGI output ready)
	BUILDING_STATIC_RESPONSE = 6,		// waiting to build response RSHIN
	SENDING_RESPONSE = 7, 		// waiting for socket to be ready for writing
	ERROR_STATE = 8
};

class Client
{
	public:
		Client();
		Client(const int client_fd, const Listener* listener);
		~Client();

		void setOwner(Listener* listener);
		void setFd(const int client_fd);

		/*Client_state*/
		bool try_parse_header();
		bool try_read_body();
		bool try_read_chunked();
		void handle_request();
		void handle_response();
		void handle_error();
		State getState() const;
		const std::string getErrorCode() const;
		void setState(const State state);
		void setErrorCode(std::string errorCode);

		
		/*Client request*/
		void select_server();
		void select_location();
		void compare_ConfReq();
		void check_permissions(const std::string& path, const struct stat& st, const std::string& method);
		std::string resolve_path();
		std::string resolve_final_path(const std::string& fs_path);
		std::string get_index_name() const;
		std::string resolve_directory(const std::string& fs_path);



		Request& getRequest();

		std::string getFullPath() const;

		/*Client buffers*/
		std::string getInBuf() const;
		std::string getOutBuf() const;
		std::string getDecodedBody() const;
		void appendIncomingData(const char *data, size_t size);
		void consumeSentData(size_t bytes);
		void setInBuf(const std::string& inbuf);
		void setOutBuf(const std::string& outbuf);

		/*Client utils*/
		void debug() const;
		int getFd() const;
		void closeFd();


		/* CGI ALEX */
		bool hasCgiRequest();
		void start_cgi();
		Cgi& getCgi();
		bool cgiHasThisPid(pid_t pid);
		bool getCgiHasExited() const;
		bool getCgiFinalized() const;
		pid_t getPid() const;
		void handle_cgi_status(int status);
		void setCgiFinalized(bool b);
		std::string getCgiOutput() const;
		bool getPendingCleanup() const;
		void setPendingCleanup(bool b);


		/* TIMEOUT */
		void updateLastActivity();
		time_t getLastActivity() const;
		void updateCgiLastActivity();
		time_t getCgiLastActivity() const;

	private:
		Client(const Client& copy);
		Client operator=(const Client& copy);

		const Listener* _owner;
		Request _req;
		Cgi _cgi;

		std::string _method;
		std::string _body;
		std::string _full_path; // for CGI script path
		std::string _in_buffer;
		std::string _out_buffer;
		std::string _decoded_body;

		int _fd;
		State _state;
		std::string _errorCode;
		size_t _headers_ends;	  // pos of \r\n\r\n
		size_t _body_start_pos;	  // headers_ends + 4
		size_t _expected_body_len; // Content-Length

		size_t _chunk_pos;		// current position
		size_t _chunk_remaining; // remaining bytes in the current chunk
		bool _header_parsed;
		bool _cgiFinalized;
		bool _pending_cleanup;
		bool _isdir;

		const Config* _selected_server;
		const LocationConfig* _selected_location;

		time_t _last_activity; // for timeout handling
		time_t _cgi_last_activity; // for CGI timeout handling

};

bool end_header(const std::string &str, size_t &headers_ends);
std::string get_extension(const std::string &path);
