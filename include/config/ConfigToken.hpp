#pragma once

#include <string>

enum ConfigTokenType
{
	ID,
	LBRACE,
	RBRACE,
	SEMICOLON,
	END
};

struct ConfigToken
{
	std::string value;
	ConfigTokenType type;
	std::string line;

	ConfigToken(const std::string& v, const enum ConfigTokenType t, const std::string& n) : value(v), type(t), line(n) {}
};
