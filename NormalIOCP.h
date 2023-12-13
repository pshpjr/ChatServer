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

	inline Session* FindSession(SessionID id, LPCWSTR content);

	unsigned short GetSessionIndex(SessionID sessionID) const { return sessionID.index; }

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

	//Memory

	MemoryUsage _memMonitor;
	CCpuUsage _cpuMonitor;

	// SESSION_MANAGER

	Session _sessions[MAX_SESSIONS];
	LockFreeStack<unsigned short> freeIndex;

	uint64 g_sessionId = 0;
	char _staticKey = 0;

	bool gracefulEnd = false;

	//GROUP_MANAGER

	GroupManager* _groupManager;
	SettingParser& _settingParser;
};

