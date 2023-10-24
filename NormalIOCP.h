#pragma once
#include "Container.h"
#include "Socket.h"
#include "Types.h"

class Session;

class NormalIOCP
{
	public:
		bool DeleteSession(SessionID id);
protected:
	void RegisterSession(Session& session);

	Session* FindSession(uint64 id);
	Session* getLockedSession(SessionID sessionID);



public:
	virtual ~NormalIOCP();

	//IOCP
	HANDLE _iocp = INVALID_HANDLE_VALUE;
	Socket _listenSocket;
	bool _isRunning = false;
	HANDLE* _threadArray = nullptr;
	uint16 _maxNetThread = 0;

	//MONITOR
	uint64 _acceptCount = 0;
	uint64 _recvCount = 0;
	uint64 _sendCount = 0;
	uint64 _acceptTps = 0;
	uint64 _recvTps = 0;
	uint64 _sendTps = 0;

// SESSION_MANAGER
	int g_id = 0;
	HashMap<uint64, Session*> _sessions;
	SRWLOCK _sessionManagerLock;
	uint64 g_sessionId = 0;
};

