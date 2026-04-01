#pragma once

#include <string>
#include <deque>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <source_location>

#include "FormatCompat.h"

//#include "Notify.h"

#ifdef min
#undef min
#endif

#ifdef WARN
#undef WARN
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
	ERR = 1 << 0, // hard error
	ITF = ERR << 1, // interface
	RTU = ITF << 1, // Routing
	ITZ = RTU << 1, // Initialize
	DVC = ITZ << 1, // Device
	ITW = DVC << 1, // Interview
	DBG = ITW << 1, // debug
	WRN = DBG << 1, // warning
};

inline constexpr eLogTypes operator|(eLogTypes a, eLogTypes b)
{
	return static_cast<eLogTypes>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline constexpr bool operator&(eLogTypes a, eLogTypes b)
{
	return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

// Forward declarations to reduce compilation dependencies
std::string LogBaseName(const char* file);
std::string MakeTag(std::source_location loc = std::source_location::current());
std::string BuildLogLine(const std::string& level, const std::string& tag, const std::string& msg);
std::tm GetLocalTime(std::time_t t);

class Logging
{
public:
	Logging(const std::string& filePath = "zwave_log.txt")
		: logFile(filePath, std::ios::out | std::ios::trunc)
	{
	}
	~Logging() { logFile.close(); }

	auto Lock() const { return std::scoped_lock(LockMutex); }

	void SetLogTypeOn(eLogTypes lt) { CurrentLogTypes |= static_cast<uint16_t>(lt); }
	void SetLogTypeOff(eLogTypes lt) { CurrentLogTypes &= ~static_cast<uint16_t>(lt); }

	struct LogEntry
	{
		std::time_t time;
		eLogTypes lt;
		std::string tag;
		std::string msg;
	};
	// returns a vector of LogEntry
	std::vector<LogEntry> GetLogEntrys(size_t last = maxEntries)
	{
		std::vector<LogEntry> result;
		{
			auto _lock = Lock();
			const size_t count = std::min(last, log.size());
			result.reserve(count);
			auto startIt = log.end() - static_cast<std::ptrdiff_t>(count);
			for (; startIt != log.end(); ++startIt)
			{
				//				if ((CurrentLogTypes & static_cast<uint16_t>(startIt.lt)) != 0)
				result.push_back(*startIt);
			}
		}
		return result;
	}

	std::vector<std::string> GetLogLines(size_t last = maxEntries)
	{
		std::vector<std::string> result;
		{
			auto _lock = Lock();
			const size_t count = std::min(last, log.size());
			result.reserve(count);
			auto startIt = log.end() - static_cast<std::ptrdiff_t>(count);
			for (; startIt != log.end(); ++startIt)
			{
				if ((CurrentLogTypes & static_cast<uint16_t>(startIt->lt)) != 0)
					result.push_back(BuildLogLine(ToString(startIt->lt), startIt->tag, startIt->msg));
			}
		}
		return result;
	}

	static std::vector<std::string> GetLevels()
	{
		return
		{
				ToString(eLogTypes::ERR),
				ToString(eLogTypes::WRN),
				ToString(eLogTypes::DVC),
				ToString(eLogTypes::ITW),
				ToString(eLogTypes::DBG),
				ToString(eLogTypes::ITF),
				ToString(eLogTypes::ITZ),
				ToString(eLogTypes::RTU),
		};
	}
	static std::string ToString(eLogTypes lt)
	{
		switch (lt)
		{
		case eLogTypes::ITZ: return "ITZ";
		case eLogTypes::ITW: return "ITW";
		case eLogTypes::DVC: return "DVC";
		case eLogTypes::ERR: return "ERR ";
		case eLogTypes::DBG: return "DBG ";
		case eLogTypes::ITF: return "ITF";
		case eLogTypes::RTU: return "RTU";
		case eLogTypes::WRN: return "WRN ";
		default: return "???";
		}
	}

	template <typename... Args>
	void AddL(eLogTypes lt, const std::string& tag, const std::string& fmt, Args&&... args)
	{
		std::string msg = FormatCompat::Format(fmt, std::forward<Args>(args)...);
		AddInternal(lt, tag, msg);
	}

	static constexpr size_t maxEntries = 2000;
private:
	mutable std::mutex LockMutex;

	std::deque<LogEntry> log;
	std::ofstream logFile;

	uint16_t CurrentLogTypes =
		static_cast<uint16_t>(eLogTypes::ERR) |
		static_cast<uint16_t>(eLogTypes::ITW) |
		static_cast<uint16_t>(eLogTypes::DVC);

	void AddInternal(eLogTypes lt, const std::string& tag, const std::string& msg)
	{
		// log all entries

		{
			auto _lock = Lock();

			auto now = std::chrono::system_clock::now();
			auto nowTime = std::chrono::system_clock::to_time_t(now);

			log.push_back({ nowTime, lt, tag, msg });
			while (log.size() > maxEntries)
				log.pop_front();
		}
		//		NotifyUI(UINotify::LogChanged);

		if (logFile.is_open())
		{
			{
				auto _lock = Lock();
				std::string line = BuildLogLine(ToString(lt), tag, msg);

				std::cout << line << std::endl;

				auto now = std::chrono::system_clock::now();
				auto t = std::chrono::system_clock::to_time_t(now);
				std::tm tmBuf = GetLocalTime(t);

				logFile << std::put_time(&tmBuf, "%H:%M:%S") << '.'
					<< std::setw(3) << std::setfill('0')
					<< std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000
					<< " " << line << std::endl;
				logFile.flush();
			}
		}
	}
};

extern Logging Log; // define in "main" as global
