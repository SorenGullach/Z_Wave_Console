#pragma once
#include <mutex>
#include <thread>
#ifndef NDEBUG
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

class DebugMutex
{
public:
	DebugMutex() = default;

	void Lock()
	{
#ifndef NDEBUG
		auto thisThread = std::this_thread::get_id();
		if (ownerThreadId == thisThread)
		{
#if defined(_MSC_VER)
			__debugbreak(); // Break on double lock in debug mode
#else
			__builtin_trap();
#endif
		}
#endif
		mtx.lock();
#ifndef NDEBUG
		ownerThreadId = std::this_thread::get_id();
#endif
	}

	void Unlock()
	{
#ifndef NDEBUG
		ownerThreadId = std::thread::id();
#endif
		mtx.unlock();
	}

	bool TryLock()
	{
		bool locked = mtx.try_lock();
#ifndef NDEBUG
		if (locked)
		{
			ownerThreadId = std::this_thread::get_id();
		}
#endif
		return locked;
	}

private:
	std::mutex mtx;
#ifndef NDEBUG
	std::thread::id ownerThreadId;
#endif
};

class DebugLockGuard
{
public:
	explicit DebugLockGuard(DebugMutex& m) : mutex(m) { mutex.Lock(); }
	~DebugLockGuard() { mutex.Unlock(); }
	DebugLockGuard(const DebugLockGuard&) = delete;
	DebugLockGuard& operator=(const DebugLockGuard&) = delete;
private:
	DebugMutex& mutex;
};
