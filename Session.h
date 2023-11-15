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
	friend class ReleaseExecutable;
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

	void SetTimeout(int timoutMillisecond) {
		_timeout = timoutMillisecond;
	}

	void CheckTimeout(chrono::system_clock::time_point now) {
		IncreaseRef(L"timeoutInc");
		if (_refCount >= releaseFlag) {
			Release(L"TimeoutRelease");
			return;
		}
		

		auto recvWait = chrono::duration_cast<chrono::milliseconds>(now - lastRecv);
		auto sendWait = chrono::duration_cast<chrono::milliseconds>(now - _postSendExecute.lastSend);

		if (needCheckSendTimeout && sendWait.count() > _timeout) {
			Close();
		}

		if (recvWait.count() > _timeout) {
			Close();
		}
		Release(L"TimeoutRelease");
	}

	long IncreaseRef(LPCWSTR content) {
		auto result =  InterlockedIncrement(&_refCount);
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index%debugSize] = { result,content,0 };
		return result;
	}
	void OffReleaseFlag() { auto result = InterlockedBitTestAndReset(&_refCount, 20); }

private:
	const unsigned long long idMask = 0x000'7FFF'FFFF'FFFF;
	const unsigned long long indexMask = 0x7FFF'8000'0000'0000;

	Socket _socket;

	CRingBuffer _recvQ;
	TLSLockFreeQueue<CSerializeBuffer*> _sendQ;

	//Executable
	RecvExecutable _recvExecute;
	PostSendExecutable _postSendExecute;
	SendExecutable _sendExecute;
	ReleaseExecutable _releaseExecutable;

	TLSLockFreeQueue<CSerializeBuffer*> _sendingQ;

	long dataNotSend = 0;

	uint64 _sessionID = 0;
	const long releaseFlag = 0x0010'0000;

	long _refCount = releaseFlag;
	long _isSending = false;

	
	bool needCheckSendTimeout = false;
	bool _connect = false;


	IOCP* _owner;


	int _timeout = 5000;
	char _staticKey = false;


	//DEBUG

	struct RelastinReleaseEncrypt_D {
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];
	
	void writeContentLog(unsigned int type) {
		release_D[InterlockedIncrement(&debugIndex)%debugSize] = { _refCount,L"SendContent",type};
	}

	chrono::system_clock::time_point lastRecv;


	//dData Debug[1000];
	//int debugIndex;
	
};
