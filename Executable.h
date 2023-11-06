#pragma once
#include <synchapi.h>
#pragma comment(lib,"Synchronization.lib")

class Session;
class Executable
{
	friend class Server;
public:
	enum ioType
	{
		BASE = 1,
		SEND,
		RECV,
		POSTRECV,
		CUSTOM
	};


	Executable() : _overlapped{0} { }
	virtual void Execute(PULONG_PTR key, DWORD transferred,void* iocp) = 0;
	virtual ~Executable() = default;

	void Clear() { memset(&_overlapped, 0, sizeof(_overlapped)); }
	LPOVERLAPPED GetOverlapped() { return &_overlapped; }

	//Executable(const Executable& other) = delete;
	//Executable(Executable&& other) noexcept = delete;
	//Executable& operator=(const Executable& other) = delete;
	//Executable& operator=(Executable&& other) noexcept = delete;
	ioType _type;
	OVERLAPPED _overlapped;
};

/**
 * \brief _execute의 인자로 key값을 넘겨주고 있다. 필요하다면 post 함수 호출할 때 넣어야 한다. 
 */
class CountExecutable :public Executable
{
public:
	CountExecutable(void* wakeupAddr, long* const Counter) : wakeupPtr(wakeupAddr), Counter(Counter){  }
	virtual ~CountExecutable() = default;

	void Execute(PULONG_PTR key, DWORD transferred, void* iocp)
	{
		_execute(key,iocp);
		long result = InterlockedDecrement(Counter);


		//if (result == 0)
		//{
		//	WakeByAddressSingle(wakeupPtr);
		//}
	}

	//void Wait()
	//{
	//		WaitOnAddress(wakeupPtr, Counter, sizeof(long), INFINITE);
	//}


protected:
	virtual void _execute(PULONG_PTR key, void* iocp) = 0;

private:
	void* wakeupPtr = nullptr;
	long* Counter = 0;
};

template <typename T>
class ExecutableManager
{
	static_assert(std::is_base_of_v<CountExecutable, T>);
public:
	ExecutableManager(int count,HANDLE iocp):Counter(count),_iocp(iocp)
	{
		_executables.reserve(count);

		for (int i = 0; i < count; ++i)
		{
				_executables.emplace_back((void*)& wakeupPtr, (long*) & Counter);
		}


	}
	void SetCounter(int count)
	{
				Counter = count;
	}
	void Run(ULONG_PTR arg)
	{
		PostQueuedCompletionStatus(_iocp, 1, arg, _executables[executablesCount++].GetOverlapped());
	}
	void Wait()
	{
		while (Counter != 0)
		{
			
		}
	}
private:
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	vector<T> _executables;
	int executablesCount = 0;
	long wakeupPtr = 0;
	volatile long Counter = 0;
};

class RecvExecutable : public Executable
{
public:
	RecvExecutable()
	{
		_type = ioType::RECV;
	}
	void Execute(PULONG_PTR key, DWORD transferred, void* iocp) override;
	~RecvExecutable() override = default;

private:
	void recvNormal(Session& session, void* iocp);
	void recvEncrypt(Session& session, void* iocp);



};

class PostSendExecutable : public Executable
{
public:
	PostSendExecutable()
	{
		_type = ioType::POSTRECV;
	}
	void Execute(PULONG_PTR key, DWORD transferred, void* iocp) override;
	~PostSendExecutable() override = default;

	bool isSend = false;
};

class SendExecutable : public Executable
{
public:
	SendExecutable()
	{
		_type = ioType::SEND;
	}
	void Execute(PULONG_PTR key, DWORD transferred, void* iocp) override;
	~SendExecutable() override = default;

};
