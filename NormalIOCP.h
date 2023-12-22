#pragma once
#include <MemoryUsage.h>
#include <CCpuUsage.h>
#include "Session.h"
#include "LockFreeStack.h"
#include "Socket.h"
#include "GroupManager.h"

class SettingParser;
class CSendBuffer;

namespace
{
	const int MAX_SESSIONS = 24000;
	const long releaseFlag = 0x0010'0000;
}

class threadMonitor
{
public:
	threadMonitor(HANDLE hThread) : _hThread(hThread)
	{
		
	}

	void Update()
	{
		FILETIME ftCreation, ftExit, ftKernel, ftUser;
		auto result = GetThreadTimes(_hThread, &ftCreation, &ftExit, &ftKernel, &ftUser);

		ULARGE_INTEGER ulKernel = FileTimeToLargeInteger(ftKernel);
		ULARGE_INTEGER ulUser = FileTimeToLargeInteger(ftUser);

		ULONGLONG kernelTime = ulKernel.QuadPart / 10000;
		ULONGLONG userTime = ulUser.QuadPart / 10000;
		
		if ( lastKernelTime > 0 )
		{
			_kernelTime = kernelTime - lastKernelTime;
			lastKernelTime = kernelTime;
		}

		if ( lastUserTime > 0 )
		{
			_userTime = userTime - lastUserTime;
			lastUserTime = userTime;
		}
	}
	ULONGLONG kernelTime() const { return _kernelTime; }
	ULONGLONG userTime() const { return _userTime; }
	ULONGLONG totTime() const { return kernelTime() + userTime(); }

private:
		ULARGE_INTEGER FileTimeToLargeInteger(const FILETIME& ft)
	{
		ULARGE_INTEGER ul;
		ul.HighPart = ft.dwHighDateTime;
		ul.LowPart = ft.dwLowDateTime;
		return ul;
	}


private:
	HANDLE _hThread;
	ULONGLONG lastKernelTime = 0;
	ULONGLONG lastUserTime = 0;

	ULONGLONG _kernelTime = 0;
	ULONGLONG _userTime = 0;
};
class NormalIOCP
{
	friend class Session;
	friend class RecvExecutable;
	friend class SendExecutable;
	friend class PostSendExecutable;
	friend class GroupExecutable;
	friend class GroupManager;
protected:
	NormalIOCP();

	inline optional<Session*> findSession(SessionID id, LPCWSTR content = L"");

	inline Session* FindSession(SessionID id, LPCWSTR content = L"")
	{
		if ( id.index > MAX_SESSIONS )
			return nullptr;

		auto& session = _sessions[id.index];
		auto result = session.IncreaseRef(content);

		//릴리즈 중이거나 세션 변경되었으면 
		if ( result > releaseFlag || session.GetSessionID().id != id.id )
		{
			//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
			session.Release(L"sessionChangedRelease");
			return nullptr;
		}

		return &session;
	}

	static unsigned short GetSessionIndex(SessionID sessionID) { return sessionID.index; }

	inline void _processBuffer(Session& session, CSendBuffer& buffer);
	void waitStart();

protected:
	virtual ~NormalIOCP();

	//IOCP
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	Socket _listenSocket;
	char _isRunning = false;
	std::vector<HANDLE> _threadArray;
	uint16 _maxNetThread = 0;
	String _ip {};
	uint16 _port {};
	int _backlog = 0;
	bool _checkTiemout = true;
	void* _this = nullptr;

	//MONITOR
	uint64 _acceptCount = 0;
	uint64 _oldAccepCount = 0;
	int64 _recvCount = 0;
	int64 _sendCount = 0;
	uint64 _oldDisconnect = 0;
	int64 _disconnectCount = 0;
	uint32 _tcpSegmenTimeout = 0;
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
	vector<threadMonitor> _threadMonitors;
	//Memory

	MemoryUsage _memMonitor;
	CCpuUsage _cpuMonitor;

	// SESSION_MANAGER

	Session _sessions[MAX_SESSIONS];
	LockFreeStack<unsigned short> freeIndex;

	uint64 g_sessionId = 0;
	char _staticKey = 0;

	char gracefulEnd = false;

	//GROUP_MANAGER

	GroupManager* _groupManager;
	SettingParser& _settingParser;
};

