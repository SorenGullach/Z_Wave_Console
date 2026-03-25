#pragma once

#include <string>
#include <string_view>

class JsonUtils
{
public:
	static std::string Escape(std::string_view s);
	static std::string_view SkipWs(std::string_view s);
	static bool TryExtractString(std::string_view json, std::string_view key, std::string& value);
	static bool TryExtractInt(std::string_view json, std::string_view key, int& value);
	static std::string MakeError(std::string_view message, std::string_view cmd);
	static std::string MakeOk(std::string_view message, std::string_view cmd);
};
