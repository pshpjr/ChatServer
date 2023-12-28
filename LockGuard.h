#pragma once

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

