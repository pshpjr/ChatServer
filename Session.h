#pragma once
#include <optional>

#include "CRingBuffer.h"
#include "Socket.h"
#include "TLSLockFreeQueue.h"
#include "LockFreeFixedQueue.h"

class CSendBuffer;
class CRecvBuffer;
class IOCP;

class RecvExecutable;
class SendExecutable;
class PostSendExecutable;
class ReleaseExecutable;

namespace SessionInfo
{
	struct SessionIdHash
	{
		std::size_t operator()(const SessionID& s) const
		{
			return std::hash<unsigned long long>()( s.id );
		}
	};

	struct SessionIdEqual
	{
		bool operator()(const SessionID& lhs, const SessionID& rhs) const
		{
			return lhs.id == rhs.id;
		}
	};
}

consteval SessionID InvalidSessionID() { return { -1 , 0}; }


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
	friend class Group;
public:
	int ioCount = 0;


	Session();
	Session(Socket socket, SessionID sessionId, IOCP& owner);
	bool CanSend();
	void Close();
	void Reset();
	bool Release(LPCWSTR content = L"", int type = 0);
	void RealSend();

	inline long IncreaseRef(LPCWSTR content)
	{
		auto result = InterlockedIncrement(&_refCount);

#ifdef SESSION_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { result,content,0 };
#endif
		return result;
	}

	void EnqueueSendData(CSendBuffer* buffer);
	void RegisterRecv();
	void RecvNotIncrease();
	void TrySend();

	void RegisterIOCP(HANDLE iocpHandle);

	SessionID GetSessionId() const { return _sessionId; }
	String GetIp() const { return _socket.GetIp(); }
	uint16 GetPort() const { return _socket.GetPort(); }

	void SetSocket(const Socket& socket) { _socket = socket; };
	void SetSessionId(const SessionID sessionID) { _sessionId = sessionID; }
	void SetOwner(IOCP& owner) { _owner = &owner; }
	void SetNetSession(const char staticKey) { _staticKey = staticKey; }
	void SetLanSession() { SetNetSession(0); }

	void SetDefaultTimeout(int timeoutMillisecond);
	void SetTimeout(int timeoutMillisecond);
	void OffReleaseFlag() { InterlockedBitTestAndReset(&_refCount, 20); }
	void SetMaxPacketLen(const int size) { _maxPacketLen = size; }
	void ResetTimeoutWait();
	void SetConnect() { _connect = true; }
	//GROUP

	GroupID GetGroupID() const { return _groupId; }

	void SetGroupID(const GroupID id)
	{
		InterlockedExchange(( long* ) &_groupId, id);
	}



private:
	bool CheckTimeout(chrono::system_clock::time_point now);

private:
	//CONST
	static constexpr unsigned long long ID_MASK = 0x000'7FFF'FFFF'FFFF;
	static constexpr unsigned long long INDEX_MASK = 0x7FFF'8000'0000'0000;
	static constexpr int MAX_SEND_COUNT = 128;

	static constexpr long RELEASE_FLAG = 0x0010'0000;

	
	//Network
	SessionID _sessionId = {{0}};
	Socket _socket;
	CRingBuffer _recvQ;
	char _recvTempBuffer[300];

	LockFreeFixedQueue<CSendBuffer*,256> _sendQ;
	//TlsLockFreeQueue<CSendBuffer*> _sendQ;
	CSendBuffer* _sendingQ[MAX_SEND_COUNT];

	//Group
	GroupID _groupId = 0;
	TlsLockFreeQueue<CRecvBuffer*> _groupRecvQ;


	IOCP* _owner;


	//Executable
	RecvExecutable& _recvExecute;
	PostSendExecutable& _postSendExecute;
	SendExecutable& _sendExecute;
	ReleaseExecutable& _releaseExecutable;


	//Reference
	long _refCount = RELEASE_FLAG;
	char _isSending = false;

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
	long debugCount = 0;
#ifdef SESSION_DEBUG

	struct RelastinReleaseEncrypt_D
	{
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

#endif


	void WriteContentLog(unsigned int type)
	{
#ifdef SESSION_DEBUG

		release_D[InterlockedIncrement(&debugIndex) % debugSize] = { _refCount,L"SendContent",type };
#endif
	}





};
