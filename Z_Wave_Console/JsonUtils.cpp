#include "JsonUtils.h"

#include <cstdio>

std::string JsonUtils::Escape(std::string_view s)
{
	std::string out;
	out.reserve(s.size() + 8);
	for (char ch : s)
	{
		switch (ch)
		{
		case '\\': out += "\\\\"; break;
		case '"': out += "\\\""; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (static_cast<unsigned char>(ch) < 0x20)
			{
				char buf[7];
				snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(ch));
				out += buf;
			}
			else
			{
				out.push_back(ch);
			}
			break;
		}
	}
	return out;
}

std::string_view JsonUtils::SkipWs(std::string_view s)
{
	while (!s.empty())
	{
		unsigned char c = static_cast<unsigned char>(s.front());
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			break;
		s.remove_prefix(1);
	}
	return s;
}

bool JsonUtils::TryExtractString(std::string_view json, std::string_view key, std::string& value)
{
	value.clear();
	const std::string needle = std::string("\"") + std::string(key) + "\"";
	size_t pos = json.find(needle);
	if (pos == std::string_view::npos)
		return false;

	pos = json.find(':', pos + needle.size());
	if (pos == std::string_view::npos)
		return false;
	pos++;

	std::string_view rest = SkipWs(json.substr(pos));
	if (rest.empty() || rest.front() != '"')
		return false;
	rest.remove_prefix(1);

	std::string out;
	out.reserve(32);
	bool escape = false;
	for (char ch : rest)
	{
		if (escape)
		{
			switch (ch)
			{
			case '"': out.push_back('"'); break;
			case '\\': out.push_back('\\'); break;
			case '/': out.push_back('/'); break;
			case 'b': out.push_back('\b'); break;
			case 'f': out.push_back('\f'); break;
			case 'n': out.push_back('\n'); break;
			case 'r': out.push_back('\r'); break;
			case 't': out.push_back('\t'); break;
			default: out.push_back(ch); break;
			}
			escape = false;
			continue;
		}

		if (ch == '\\')
		{
			escape = true;
			continue;
		}
		if (ch == '"')
		{
			value = std::move(out);
			return true;
		}
		out.push_back(ch);
	}

	return false;
}

bool JsonUtils::TryExtractInt(std::string_view json, std::string_view key, int& value)
{
	const std::string needle = std::string("\"") + std::string(key) + "\"";
	size_t pos = json.find(needle);
	if (pos == std::string_view::npos)
		return false;

	pos = json.find(':', pos + needle.size());
	if (pos == std::string_view::npos)
		return false;
	pos++;

	std::string_view rest = SkipWs(json.substr(pos));
	if (rest.empty())
		return false;

	int sign = 1;
	size_t i = 0;
	if (rest[i] == '-')
	{
		sign = -1;
		i++;
	}
	if (i >= rest.size() || rest[i] < '0' || rest[i] > '9')
		return false;

	long long n = 0;
	for (; i < rest.size(); i++)
	{
		char ch = rest[i];
		if (ch < '0' || ch > '9')
			break;
		n = (n * 10) + (ch - '0');
		if (n > 2147483647LL)
			break;
	}
	value = static_cast<int>(sign * n);
	return true;
}

std::string JsonUtils::MakeError(std::string_view message, std::string_view cmd)
{
	return std::string("{\"type\":\"error\",\"message\":\"") + Escape(message) +
		"\",\"cmd\":\"" + Escape(cmd) + "\"}";
}
