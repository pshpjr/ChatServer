#pragma once
#include "CRingBuffer.h"
#include "Executable.h"
#include "Socket.h"

class IOCP;
class SessionManager;
class CSerializeBuffer;

class Session
{
	friend class Executable;
	friend class RecvExecutable;
	friend class PostSendExecutable;
	friend class SendExecutable;
	friend class IOCP;
public:
	Session(Socket socket, uint64 sessionId, IOCP& owner);
	void Enqueue(CSerializeBuffer* buffer);
	void Close();
	bool Release();
	void Lock() { EnterCriticalSection(&_lock); }
	void Unlock() { LeaveCriticalSection(&_lock); }
	void trySend();
	void registerRecv();
	void _postRecvNotIncrease();
	void RegisterIOCP(HANDLE iocpHandle);
	uint64 GetSessionID() { return _sessionID; } 
private:

	Socket _socket;

	CRingBuffer _recvQ;
	CRingBuffer _sendQ;

	RecvExecutable _recvExecute;
	PostSendExecutable _postSendExecute;
	SendExecutable _sendExecute;


	int sendingSerializeBuffers=0;
	long dataNotSend = 0;
	uint64 _sessionID = 0;
	long _refCount = 0;
	long _isSending = false;
	CRITICAL_SECTION _lock;
	bool _isDisconnected = false;
	//세션 여기저기 옮기지 못 하게 하나로 고정.
	IOCP& _owner;
};
