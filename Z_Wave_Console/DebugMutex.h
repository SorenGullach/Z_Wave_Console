#pragma once
#include <mutex>
#include <thread>
#ifdef _DEBUG
#include <intrin.h>
#endif

class DebugMutex
{
public:
	DebugMutex() : ownerThreadId() {}

	void Lock()
	{
#ifdef _DEBUG
		auto thisThread = std::this_thread::get_id();
		if (ownerThreadId == thisThread)
		{
			__debugbreak(); // Break on double lock in debug mode
		}
#endif
		mtx.lock();
#ifdef _DEBUG
		ownerThreadId = std::this_thread::get_id();
#endif
	}

	void Unlock()
	{
#ifdef _DEBUG
		ownerThreadId = std::thread::id();
#endif
		mtx.unlock();
	}

	bool TryLock()
	{
		bool locked = mtx.try_lock();
#ifdef _DEBUG
		if (locked)
		{
			ownerThreadId = std::this_thread::get_id();
		}
#endif
		return locked;
	}

private:
	std::mutex mtx;
#ifdef _DEBUG
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
