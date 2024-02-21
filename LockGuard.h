#pragma once
#include <shared_mutex>

class SharedLockGuard
{
public:
	SharedLockGuard(const SharedLockGuard& other) = delete;
	SharedLockGuard(SharedLockGuard&& other) = delete;
	SharedLockGuard& operator=(const SharedLockGuard& other) = delete;
	SharedLockGuard& operator=(SharedLockGuard&& other) = delete;

	explicit SharedLockGuard(SRWLOCK& lock) : _cs(lock)
	{
		AcquireSRWLockShared(&_cs);
	}
	~SharedLockGuard()
	{
		ReleaseSRWLockShared(&_cs);
	}

private:
	SRWLOCK& _cs;
};

class ExclusiveLockGuard
{
public:
	ExclusiveLockGuard(const ExclusiveLockGuard& other) = delete;
	ExclusiveLockGuard(ExclusiveLockGuard&& other) = delete;
	ExclusiveLockGuard& operator=(const ExclusiveLockGuard& other) = delete;
	ExclusiveLockGuard& operator=(ExclusiveLockGuard&& other) = delete;

	ExclusiveLockGuard(SRWLOCK& lock) : _cs(lock)
	{
		AcquireSRWLockExclusive(&_cs);
	}
	~ExclusiveLockGuard()
	{
		ReleaseSRWLockExclusive(&_cs);
	}
private:
	SRWLOCK& _cs;
};

#pragma once

class ReadLockGuard
{
public:
	ReadLockGuard(const ReadLockGuard& other) = delete;
	ReadLockGuard(ReadLockGuard&& other) = delete;
	ReadLockGuard& operator=(const ReadLockGuard& other) = delete;
	ReadLockGuard& operator=(ReadLockGuard&& other) = delete;

	explicit ReadLockGuard(shared_mutex& lock) : _cs(lock)
	{
		_cs.lock_shared();
	}
	~ReadLockGuard()
	{
		_cs.unlock_shared();
	}

private:
	std::shared_mutex& _cs;
};

class WriteLockGuard
{
public:
	WriteLockGuard(const WriteLockGuard& other) = delete;
	WriteLockGuard(WriteLockGuard&& other) = delete;
	WriteLockGuard& operator=(const WriteLockGuard& other) = delete;
	WriteLockGuard& operator=(WriteLockGuard&& other) = delete;

	WriteLockGuard(shared_mutex& lock) : _cs(lock)
	{
		_cs.lock();
	}
	~WriteLockGuard()
	{
		_cs.unlock();
	}
private:
	std::shared_mutex& _cs;
};

