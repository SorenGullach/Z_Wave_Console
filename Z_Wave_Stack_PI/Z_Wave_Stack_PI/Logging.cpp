#include "pch.h"
#include "Logging.h"

std::string LogBaseName(const char* file)
{
	std::string s(file);
	auto pos = s.find_last_of("\\/");
	if (pos == std::string::npos)
		pos = 0;
	else
		++pos;
	auto pos1 = s.length();
	return s.substr(pos, pos1 - pos);
}

std::string MakeTag(std::source_location loc)
{
#ifndef NDEBUG
	return LogBaseName(loc.file_name()) + ":" + std::to_string(loc.line());
#else
	return LogBaseName(loc.file_name());
#endif
}

std::string BuildLogLine(const std::string& level, const std::string& tag, const std::string& msg)
{
	std::ostringstream stream;
	stream << std::left << std::setw(3) << level
		<< " [" << std::setw(20) << tag << "] " << msg;
	return stream.str();
}

std::tm GetLocalTime(std::time_t t)
{
	std::tm tmBuf{};
#if defined(_WIN32)
	localtime_s(&tmBuf, &t);
#else
	localtime_r(&t, &tmBuf);
#endif
	return tmBuf;
}
