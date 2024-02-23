#pragma once
#include <MemoryUsage.h>
#include <CCpuUsage.h>
#include "Session.h"
#include "LockFreeStack.h"
#include "Socket.h"
#include "GroupManager.h"

class SettingParser;
class CSendBuffer;



class threadMonitor
{
public:
	threadMonitor(const HANDLE hThread) : _hThread(hThread)
	{
		
	}

	void Update()
	{
		FILETIME ftCreation, ftExit, ftKernel, ftUser;
		GetThreadTimes(_hThread, &ftCreation, &ftExit, &ftKernel, &ftUser);

		const ULARGE_INTEGER ulKernel = FileTimeToLargeInteger(ftKernel);
		const ULARGE_INTEGER ulUser = FileTimeToLargeInteger(ftUser);

		const ULONGLONG kernelTime = ulKernel.QuadPart / 10000;
		const ULONGLONG userTime = ulUser.QuadPart / 10000;
		
		if ( _lastKernelTime > 0 )
		{
			_kernelTime = kernelTime - _lastKernelTime;
			_lastKernelTime = kernelTime;
		}

		if ( _lastUserTime > 0 )
		{
			_userTime = userTime - _lastUserTime;
			_lastUserTime = userTime;
		}
	}
	ULONGLONG KernelTime() const { return _kernelTime; }
	ULONGLONG UserTime() const { return _userTime; }
	ULONGLONG TotTime() const { return KernelTime() + UserTime(); }

private:
	ULARGE_INTEGER FileTimeToLargeInteger(const FILETIME& ft)
	{
		ULARGE_INTEGER ul {};
		ul.HighPart = ft.dwHighDateTime;
		ul.LowPart = ft.dwLowDateTime;
		return ul;
	}


private:
	HANDLE _hThread;
	ULONGLONG _lastKernelTime = 0;
	ULONGLONG _lastUserTime = 0;

	ULONGLONG _kernelTime = 0;
	ULONGLONG _userTime = 0;
};

class NormalIOCP
{
public:
	NormalIOCP(const NormalIOCP& other) = delete;
	NormalIOCP(NormalIOCP&& other) = delete;
	NormalIOCP& operator=(const NormalIOCP& other) = delete;
	NormalIOCP& operator=(NormalIOCP&& other) = delete;

protected:
	NormalIOCP();

	inline optional<Session*> findSession(SessionID id, LPCWSTR content = L"");

	inline Session* FindSession(const SessionID id, const LPCWSTR content = L"")
	{
		if ( id.index > MAX_SESSIONS || id.index <0 )
		{
			return nullptr;
		}

		auto& session = _sessions[id.index];

		//릴리즈 중이거나 세션 변경되었으면 
		if (session.IncreaseRef(content) > releaseFlag || session.GetSessionId().id != id.id )
		{
			//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
			session.Release(L"sessionChangedRelease");
			return nullptr;
		}

		return &session;
	}

	static unsigned short GetSessionIndex(const SessionID sessionID) { return sessionID.index; }

	static inline void ProcessBuffer(Session& session, CSendBuffer& buffer);
	void WaitStart();

protected:
	static constexpr int MAX_SESSIONS = 15000;
	static constexpr long releaseFlag = 0x0010'0000;

	virtual ~NormalIOCP();


	/*
	*
	*	IOCP
	*
	*/

	HANDLE _iocp = INVALID_HANDLE_VALUE;
	Socket _listenSocket;

	std::vector<std::pair<HANDLE,int>> _threadArray;

	String _ip {};
	uint16 _maxNetThread = 0;
	uint16 _maxWorkerThread = 0; 
	uint16 _port {};
	int _backlog = 0;

	bool _checkTiemout = true;
	void* _this = nullptr;
	char _staticKey = 0;
	char gracefulEnd = false;
	char _isRunning = false;

	/*
	*
	*	MONITOR
	*
	*/
	uint64 _acceptCount = 0;
	uint64 _oldAccepCount = 0;
	int64 _recvCount = 0;
	int64 _sendCount = 0;
	uint64 _oldDisconnect = 0;
	int64 _disconnectCount = 0;
	uint32 _tcpSemaphoreTimeout = 0;
	uint64 _acceptTps = 0;
	uint64 _recvTps = 0;
	uint64 _sendTps = 0;
	uint64 _disconnectps = 0;
	short _sessionCount = 0;
	uint64 _packetPoolSize = 0;
	uint32 _packetPoolEmpty = 0;
	uint64 _timeoutSessions = 0;
	uint32 _acceptErrorCount = 0;
	long _iocpCompBufferSize = 0;

	//vector<threadMonitor> _threadMonitors;
	MemoryUsage _memMonitor;
	CCpuUsage _cpuMonitor;




	/*
	*
	*	SESSION_MANAGER
	*
	*/
	Session _sessions[MAX_SESSIONS];
	LockFreeStack<unsigned short> freeIndex;
	uint64 g_sessionId = 0;

	/*
	*
	*	GROUP_MANAGER
	*
	*/

	GroupManager* _groupManager;
	SettingParser& _settingParser;
};

