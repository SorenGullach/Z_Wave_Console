#pragma once

#include <string>
#include <deque>
#include <format>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <source_location>
#include <algorithm>

#ifdef min
#undef min
#endif

// ===============================================================
// Thread-safe logging helper
//
// Keeps a bounded in-memory log (ring buffer) and writes the same entries
// to a text file for offline debugging.
//
// `MakeTag()` uses `std::source_location` to generate a compact tag.
// ===============================================================

enum class eLogTypes
{
	ERR = 1,
	INFO = 2,
	DBG = 3,
};

inline std::string ZW_BaseName(const char* file)
{
	std::string s(file);
	auto pos = s.rfind('\\');
	auto pos1 = s.length(); // .rfind('.');
	return s.substr(pos + 1, pos1 - pos - 1);
}

inline std::string MakeTag(std::source_location loc = std::source_location::current())
{
#ifdef _DEBUG
	return std::format("{}:{}", ZW_BaseName(loc.file_name()), loc.line());
#else
	return ZW_BaseName(loc.file_name());
#endif
}

class ZW_Logging
{
public:
	ZW_Logging(const std::string& filePath = "zwave_log.txt")
		: logFile(filePath, std::ios::out | std::ios::trunc)
	{}
	~ZW_Logging() { logFile.close(); }

	auto Lock() const { return std::scoped_lock(LockMutex); }

	void SetLogType(eLogTypes lt) { CurrentLogType = lt; }

	std::vector<std::string> GetLog(size_t last = maxEntries)
	{
		std::vector<std::string> result;
		{
			auto _lock = Lock();

			const size_t count = std::min(last, log.size());
			result.resize(count);
			const auto startIt = log.end() - static_cast<std::ptrdiff_t>(count);
			std::copy(startIt, log.end(), result.begin());
		}
		return result;
	}

	template <typename... Args>
	void AddL(eLogTypes lt, const std::string& tag, std::format_string<Args...> fmt, Args&&... args)
	{
		std::string msg = std::format(fmt, std::forward<Args>(args)...);
		AddInternal(lt, tag, msg);
	}

private:
	mutable std::mutex LockMutex;

	static constexpr size_t maxEntries = 100;
	std::deque<std::string> log;
	std::ofstream logFile;

	std::string ToString(eLogTypes lt) const
	{
		switch (lt)
		{
		case eLogTypes::INFO: return "INFO";
		case eLogTypes::ERR: return "ERR ";
		case eLogTypes::DBG: return "DBG ";
		default: return "???";
		}
	}


	eLogTypes CurrentLogType = eLogTypes::INFO;
	std::vector<std::string> GetLogLast(size_t last)
	{
		auto _lock = Lock();

		const size_t count = std::min(last, log.size());
		std::vector<std::string> result(count);
		const auto startIt = log.end() - static_cast<std::ptrdiff_t>(count);
		std::copy(startIt, log.end(), result.begin());
		return result;
	}

	void AddInternal(eLogTypes lt, const std::string& tag, const std::string& msg)
	{
		if (CurrentLogType < lt)
			return;

		std::string line = std::format("{:3} [{:30}] {}", ToString(lt), tag, msg);
		{
			auto _lock = Lock();

			log.push_back({ line });
			while (log.size() > maxEntries)
				log.pop_front();
		}

		if (logFile.is_open())
		{
			{
				auto now = std::chrono::system_clock::now();
				auto t = std::chrono::system_clock::to_time_t(now);
				std::tm tmBuf{};
				localtime_s(&tmBuf, &t);

				logFile << std::put_time(&tmBuf, "%H:%M:%S") << '.' 
				        << std::setw(3) << std::setfill('0') 
				        << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000
				        << " " << line << std::endl;
				logFile.flush();
			}
		}
	}
};

extern ZW_Logging Log; // define in "main" as global
