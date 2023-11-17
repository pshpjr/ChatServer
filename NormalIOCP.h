#pragma once
#include "Container.h"
#include "Socket.h"
#include "Types.h"
#include "LockFreeStack.h"
#include "Session.h"

class NormalIOCP
{
	friend class Session;
	friend class RecvExecutable;
	friend class SendExecutable;
	friend class PostSendExecutable;
public:

protected:

	Session* FindSession(uint64 id,  LPCWSTR content);

	unsigned short GetSessionIndex(uint64 sessionID) const { return (unsigned short)(sessionID >> 47); }

protected:
	virtual ~NormalIOCP();

	//IOCP
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	Socket _listenSocket;
	bool _isRunning = false;
	vector<HANDLE> _threadArray;
	uint16 _maxNetThread = 0;
	String _ip;
	uint16 _port;
	bool _checkTiemout = true;
	
	//MONITOR
	uint64 _acceptCount = 0;
	uint64 _oldAccepCount = 0;
	uint64 _recvCount = 0;
	uint64 _sendCount = 0;
	uint64 _acceptTps = 0;
	uint64 _recvTps = 0;
	uint64 _sendTps = 0;
	short _sessionCount = 0;
	uint64 _packetPoolSize = 0;
	uint32 _packetPoolEmpty = 0;
	uint64 _timeoutSessions = 0;

// SESSION_MANAGER
	int g_id = 0;
	static const int MAX_SESSIONS = 16000;
	Session sessions[MAX_SESSIONS];
	LockFreeStack<short> freeIndex;
	const unsigned long long idMask = 0x000'7FFF'FFFF'FFFF;
	const unsigned long long indexMask = 0x7FFF'8000'0000'0000;
	const long releaseFlag = 0x0010'0000;




	uint64 g_sessionId = 0;
	char _staticKey;

	bool gracefulEnd = false;



};

