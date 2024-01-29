#pragma once
#include "Container.h"


namespace executable {
	enum ExecutableTransfer
	{
		BASE = 1,
		POST,
		RECV,
		SEND,
		RELEASE,
		WAIT,
		GROUP
	};
}

class Executable
{
public:
	enum ioType
	{
		BASE = 1,
		SEND,
		RECV,
		POSTRECV,
		RELEASE,
		CUSTOM
	};
	Executable() :_type(CUSTOM), _overlapped{0} { }
	virtual void Execute(ULONG_PTR arg, DWORD transferred,void* iocp) = 0;
	virtual ~Executable() = default;

	static Executable* GetExecutable(OVERLAPPED* overlap)
	{
		return (Executable*)((char*)overlap - offsetof(Executable,_overlapped));
	}
	void Clear() { memset(&_overlapped, 0, sizeof(_overlapped)); }
	LPOVERLAPPED GetOverlapped() { return &_overlapped; }

	//Executable(const Executable& other) = delete;
	//Executable(Executable&& other) noexcept = delete;
	//Executable& operator=(const Executable& other) = delete;
	//Executable& operator=(Executable&& other) noexcept = delete;
	ioType _type;
	OVERLAPPED _overlapped;
};

class WaitExecutable :public Executable
{
public:
	virtual ~WaitExecutable() = default;

	void Execute(const ULONG_PTR key, DWORD transferred, void* iocp)
	{
		_execute(key, iocp);

		isDone = true;
		WakeByAddressSingle(&isDone);
	}

	void Join()
	{
		char expect = false;
		WaitOnAddress(&isDone, &expect, sizeof(long), INFINITE);
	}

protected:
	virtual void _execute(ULONG_PTR key, void* iocp) = 0;

private:
	char isDone = false;
};

template <typename T>
class ExecutableManager
{
	static_assert( std::is_base_of_v<WaitExecutable, T> );
public:
	ExecutableManager(int count, const HANDLE iocp) :_iocp(iocp)
	{
		_executables.resize(count);
	}

	void Run(const ULONG_PTR arg)
	{

		for ( auto& exe : _executables )
		{
			int transfered = 1;
			PostQueuedCompletionStatus(_iocp, transfered, arg, exe.GetOverlapped());

		}
	}
	void Wait()
	{
		for ( auto& exe : _executables )
		{
			exe.Join();
		}
	}
private:
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	vector<T> _executables;
};




