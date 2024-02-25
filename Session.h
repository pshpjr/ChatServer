#pragma once
#include <optional>
#include <chrono>
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

consteval SessionID InvalidSessionID() { return { -1 , 0}; }

struct timeoutInfo
{
	enum class IOtype
	{
		recv,
		send
	};

	IOtype type;
	long long waitTime = 0;
	int timeoutTime = 0;
};

//#define SESSION_DEBUG
class Session
{
	friend class RecvExecutable;
	friend class PostSendExecutable;
	friend class SendExecutable;
	friend class ReleaseExecutable;
	friend class IOCP;
	friend class Group;
public:



	Session();
	Session(Socket socket, SessionID sessionId, IOCP& owner);

	void Close();
	void Reset();
	void PostRelease();


	inline long IncreaseRef(LPCWSTR content);
	bool Release(LPCWSTR content = L"", int type = 0);



	void EnqueueSendData(CSendBuffer* buffer);
	void RegisterRecv();
	void RecvNotIncrease();
	void TrySend();
	bool CanSend();


	void RegisterIOCP(HANDLE iocpHandle);

	inline SessionID GetSessionId() const { return _sessionId; }
	String GetIp() const { return _socket.GetIp(); }
	uint16 GetPort() const { return _socket.GetPort(); }

	void SetSocket(const Socket& socket) { _socket = socket; };
	void SetSessionId(const SessionID sessionID) { _sessionId = sessionID; }
	void SetOwner(IOCP& owner) { _owner = &owner; }
	void SetNetSession(const char staticKey) { _staticKey = staticKey; }
	void SetLanSession() { SetNetSession(0); }
	void SetMaxPacketLen(const int size) { _maxPacketLen = size; }
	void SetConnect() { _connect = true; }

	void SetDefaultTimeout(int timeoutMillisecond);
	void SetTimeout(int timeoutMillisecond);
	void OffReleaseFlag() { InterlockedBitTestAndReset(&_refCount, 20); }

	void ResetTimeoutWait();

	//0이면 아무 그룹 아님. 
	inline GroupID GetGroupID() const { return _groupId; }
	inline void SetGroupID(const GroupID id)
	{
		InterlockedExchange(( long* ) &_groupId, id);
	}

private:
	std::optional<timeoutInfo> CheckTimeout(std::chrono::steady_clock::time_point now);
	void RealSend();

private:

	long GetRefCount(SessionID id) {
		//auto value = _refCount;
		auto value =  InterlockedOr(&_refCount, 0); 
		ASSERT_CRASH(_sessionId == id);
		return value;
	}
	//CONST
	static constexpr unsigned long long ID_MASK = 0x000'7FFF'FFFF'FFFF;
	static constexpr unsigned long long INDEX_MASK = 0x7FFF'8000'0000'0000;
	static constexpr int MAX_SEND_COUNT = 256;

	static constexpr long RELEASE_FLAG = 0x0010'0000;

	
	//Network	
	alignas(64) SessionID _sessionId = { {0} };
	Socket _socket;
	GroupID _groupId = GroupID(0);
	long _refCount = RELEASE_FLAG;
#ifdef SESSION_DEBUG
	long DebugRef = 0;
#endif
	char _connect = false;

	LockFreeFixedQueue<CSendBuffer*, MAX_SEND_COUNT> _sendQ;
	//TlsLockFreeQueue<CSendBuffer*> _sendQ;
	CSendBuffer* _sendingQ[MAX_SEND_COUNT];


	//TlsLockFreeQueue<CRecvBuffer*> _groupRecvQ;

	IOCP* _owner;


	//Executable
	RecvExecutable& _recvExecute;
	PostSendExecutable& _postSendExecute;
	SendExecutable& _sendExecute;
	ReleaseExecutable& _releaseExecutable;


	//Reference
	char _isSending = false;

	//Timeout
	bool _needCheckSendTimeout = false;

	int _defaultTimeout = 5000;
	int _timeout = 5000;
	CRingBuffer _recvQ;
	std::chrono::steady_clock::time_point lastRecv;

	//Encrypt
	char _staticKey = false;
	unsigned int _maxPacketLen = 5000;

	//DEBUG
	long debugCount = 0;
#ifdef SESSION_DEBUG

	struct RelastinReleaseEncrypt_D
	{
		SessionID id;
		GroupID dst = InvalidGroupID();
		long refCount;
		LPCWSTR location;
		GroupID groupId = InvalidGroupID();
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

	//세션 id, 종류
	//tuple<SessionID, String, thread::id> _debugLeave[debugSize];
	public:
	void Write(long refCount, GroupID dst, LPCWSTR cause)
	{
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { GetSessionId(),dst,refCount, cause,GetGroupID()};
	}

#endif


	void WriteContentLog(unsigned int type)
	{
#ifdef SESSION_DEBUG
		Write(_refCount, InvalidGroupID(), L"SendContent");
#endif
	}

};


long Session::IncreaseRef(LPCWSTR Content)
{
	auto result = InterlockedIncrement(&_refCount);

#ifdef SESSION_DEBUG
	Write(_refCount, InvalidGroupID(), Content);

#endif
	return result;
}

