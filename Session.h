#pragma once
#include "CRingBuffer.h"
#include "Executable.h"
#include "LockFreeQueue.h"
#include "Socket.h"
#include "TLSLockFreeQueue.h"

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

	Session();
	Session(Socket socket, uint64 sessionId, IOCP& owner);
	void Enqueue(CSerializeBuffer* buffer);
	void Close();
	bool Release(LPCWSTR content,int type = 0);
	void trySend();
	void registerRecv();
	void RecvNotIncrease();
	void RegisterIOCP(HANDLE iocpHandle);
	uint64 GetSessionID() { return _sessionID; }
	void SetSocket(Socket socket) { _socket = socket; };
	void SetSessionID(uint64 sessionID) { _sessionID = sessionID; }
	void SetOwner(IOCP& owner) { _owner = &owner; }
	void Reset();
	void SetNetSession(char staticKey) { _staticKey = staticKey; }
	long IncreaseRef(LPCWSTR content) {
		auto result =  InterlockedIncrement(&_refCount);
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index%debugSize] = { result,content,_sessionID };
		return result;
	}
	void OffReleaseFlag() { auto result = InterlockedBitTestAndReset(&_refCount, 20); }

private:
	const unsigned long long idMask = 0x000'7FFF'FFFF'FFFF;
	const unsigned long long indexMask = 0x7FFF'8000'0000'0000;

	Socket _socket;

	CRingBuffer _recvQ;
	TLSLockFreeQueue<CSerializeBuffer*> _sendQ;

	RecvExecutable _recvExecute;
	PostSendExecutable _postSendExecute;
	SendExecutable _sendExecute;

	TLSLockFreeQueue<CSerializeBuffer*> _sendingQ;

	long dataNotSend = 0;
	uint64 _sessionID = 0;
	long _refCount = 0;
	long _isSending = false;
	CRITICAL_SECTION _lock;
	bool _disconnect = false;
	//세션 여기저기 옮기지 못 하게 하나로 고정.
	IOCP* _owner;
	const long releaseFlag = 0x0010'0000;

	char _staticKey = false;

	struct RelastinReleaseEncrypt_D {
		long refCount;
		LPCWSTR location;
		unsigned long long contentType;
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];


	//dData Debug[1000];
	//int debugIndex;
	
};
