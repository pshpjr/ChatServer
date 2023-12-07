#pragma once

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
		RELEASE,
		CUSTOM
	};
	Executable() :_type(BASE), _overlapped{0} { }
	virtual void Execute(ULONG_PTR arg, DWORD transferred,void* iocp) = 0;
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

class PostSendExecutable : public Executable
{
public:
	PostSendExecutable()
	{
		_type = ioType::POSTRECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~PostSendExecutable() override = default;

	std::chrono::system_clock::time_point lastSend;
};

class RecvExecutable : public Executable
{
public:
	RecvExecutable()
	{
		_type = ioType::RECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~RecvExecutable() override = default;

private:
	void recvNormal(Session& session, void* iocp);
	void recvEncrypt(Session& session, void* iocp);

	template <typename Header>
	void recvHandler(Session& session, void* iocp);
};


class SendExecutable : public Executable
{
public:
	SendExecutable()
	{
		_type = ioType::SEND;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~SendExecutable() override = default;

};

class ReleaseExecutable : public Executable
{
public:
	ReleaseExecutable()
	{
		_type = ioType::RELEASE;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~ReleaseExecutable() override = default;

};




class WaitExecutable :public Executable
{
public:
	virtual ~WaitExecutable() = default;

	void Execute(ULONG_PTR key, DWORD transferred, void* iocp)
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
	ExecutableManager(int count, HANDLE iocp) :_iocp(iocp)
	{
		_executables.resize(count);
	}

	void Run(ULONG_PTR arg)
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




