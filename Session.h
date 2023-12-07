#pragma once
#include "CRingBuffer.h"
#include "Socket.h"
#include "TLSLockFreeQueue.h"
#include "Executable.h"

class CSendBuffer;
class IOCP;
namespace 
{
	const unsigned long long ID_MASK = 0x000'7FFF'FFFF'FFFF;
	const unsigned long long INDEX_MASK = 0x7FFF'8000'0000'0000;
	const int MAX_SEND_COUNT = 128;
	const long RELEASE_FLAG = 0x0010'0000;
}

//#define SESSION_DEBUG
class Session
{
	friend class Executable;
	friend class RecvExecutable;
	friend class PostSendExecutable;
	friend class SendExecutable;
	friend class ReleaseExecutable;
	friend class IOCP;
	friend class NormalIOCP;

public:

	char* _debugRecvPtr = nullptr;

	Session();
	Session(Socket socket, uint64 sessionId, IOCP& owner);
	void Close();
	void Reset();
	bool Release(LPCWSTR content, int type = 0);

	long IncreaseRef(LPCWSTR content)
	{
		auto result = InterlockedIncrement(&_refCount);

#ifdef SESSION_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { result,content,0 };
#endif
		return result;
	}

	void EnqueueSendData(CSendBuffer* buffer);
	void registerRecv();
	void RecvNotIncrease();
	void trySend();

	void RegisterIOCP(HANDLE iocpHandle);

	uint64 GetSessionID() const { return _sessionID; }
	String GetIP() const { return _socket.GetIP(); }
	uint16 GetPort() const { return _socket.GetPort(); }

	void SetSocket(Socket socket) { _socket = socket; };
	void SetSessionID(uint64 sessionID) { _sessionID = sessionID; }
	void SetOwner(IOCP& owner) { _owner = &owner; }
	void SetNetSession(char staticKey) { _staticKey = staticKey; }
	void SetDefaultTimeout(int timoutMillisecond);
	void SetTimeout(int timoutMillisecond);
	void OffReleaseFlag() { auto result = InterlockedBitTestAndReset(&_refCount, 20); }
	void SetMaxPacketLen(int size) { _maxPacketLen = size; }
	void ResetTimeoutwait();

private:
	bool CheckTimeout(chrono::system_clock::time_point now);



private:
	//Network
	uint64 _sessionID = 0;
	Socket _socket;
	CRingBuffer _recvQ;

	TLSLockFreeQueue<CSendBuffer*> _sendQ;
	CSendBuffer* _sendingQ[MAX_SEND_COUNT];
	long dataNotSend = 0;

	IOCP* _owner;


	//Executable
	RecvExecutable _recvExecute;
	PostSendExecutable _postSendExecute;
	SendExecutable _sendExecute;
	ReleaseExecutable _releaseExecutable;


	//Reference
	long _refCount = RELEASE_FLAG;
	long _isSending = false;

	//Timeout
	bool needCheckSendTimeout = false;
	char _connect = false;
	int _defaultTimeout = 5000;
	int _timeout = 5000;

	chrono::system_clock::time_point lastRecv;

	//Encrypt
	char _staticKey = false;
	unsigned int _maxPacketLen = 20000;

	//DEBUG
#ifdef SESSION_DEBUG

	struct RelastinReleaseEncrypt_D {
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

#endif


	void writeContentLog(unsigned int type) {
#ifdef SESSION_DEBUG

		release_D[InterlockedIncrement(&debugIndex)%debugSize] = { _refCount,L"SendContent",type};
#endif
	}




	
};
