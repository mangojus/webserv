#pragma once
#include <string>
#include <map>

class Request
{
private:
	std::string method;
	std::string path;
	std::string query;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
	size_t content_len;
	bool has_content_length;
	bool chunked;

public:
	Request();
	void setMethod(const std::string &m);
	void setPath(const std::string &p);
	void setVersion(const std::string &v);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &b);
	void setContent_len(const size_t &c);
	void setHas_content_length(const bool &c);
	void setChunked(const bool &c);
	void setQuery(const std::string &q);

	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::string &getBody() const;
	std::string getHeader(const std::string &key) const;
	const size_t &getContent_len() const;
	const bool &getHas_content_length() const;
	const bool &getChunked() const;
	const std::string &getQuery() const;

	// debug
	std::string getHeaders(void) const;

	void debug() const;

	// alex
	std::string content_type;
};
