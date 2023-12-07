#pragma once
#include "Container.h"
#include "Socket.h"
#include "Types.h"
#include "LockFreeStack.h"
#include "Session.h"
#include "CCpuUsage.h"
#include "MemoryUsage.h"
class SendBuffer;
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
protected:

	inline Session* FindSession(uint64 id,  LPCWSTR content);

	unsigned short GetSessionIndex(uint64 sessionID) const { return (unsigned short)(sessionID >> 47); }

	inline void _processBuffer(Session& session, CSendBuffer& buffer);
	void waitStart();
	
protected:
	virtual ~NormalIOCP();

	//IOCP
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	Socket _listenSocket;
	char _isRunning = false;
	vector<HANDLE> _threadArray;
	uint16 _maxNetThread = 0;
	String _ip;
	uint16 _port;
	bool _checkTiemout = true;
	
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
public:

};

