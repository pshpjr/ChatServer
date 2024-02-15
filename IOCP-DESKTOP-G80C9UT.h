#pragma once
#include "BuildOption.h"
#include IOCP_HEADER

class SendBuffer;
class CRecvBuffer;
class Executable;
class Client;




template <typename T>
class SingleThreadQ;
class IOCP : public IOCP_CLASS
{
	friend class RecvExecutable;
	friend class ReleaseExecutable;
	friend class PostSendExecutable;
	friend class Session;
	friend class Group;
	friend class GroupManager;
	friend class Client;
public:
	IOCP();

	bool Init(const String& ip, Port port, uint16 backlog, uint16 maxRunningThread,uint16 workerThread, char staticKey);
	void Start();
	void Stop();
	void Wait();
	bool SendPacket(SessionID sessionId, SendBuffer& sendBuffer, int type = 0);
	bool SendPacketBlocking(SessionID sessionId, SendBuffer& sendBuffer, int type = 0);
	//deprecate
	bool SendPackets(SessionID sessionId, SingleThreadQ<CSendBuffer*>& bufArr);

	static void SetDisableClickAndClose();
	bool DisconnectSession(SessionID sessionId);
	bool isEnd() const;
	void SetMaxPacketSize(int size);
	void SetTimeout(SessionID sessionId, int timeoutMillisecond);
	void SetDefaultTimeout(unsigned int timeoutMillisecond);
	void PostExecutable(Executable* exe, ULONG_PTR arg) const;

	void SetSessionStaticKey(SessionID id, char staticKey);
	
	//connect
	WSAResult<SessionID> GetClientSession(const String& ip, Port port);
	bool IsValidSession(SessionID id);
	//GROUP


	//CONTENT VIRTUAL

	virtual void OnWorkerThreadBegin() {}
	virtual void OnWorkerThreadEnd() {}
	virtual bool OnAccept(SockAddr_in) { return true; }
	virtual void OnConnect(SessionID sessionId, const SockAddr_in& info) {}
	virtual void OnDisconnect(SessionID sessionId) {}
	virtual void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) {}
	virtual void OnInit() {}
	virtual void OnStart() {}
	virtual void OnEnd() {}

	virtual void OnSessionTimeout(SessionID sessionId, timeoutInfo info) {}
	virtual void OnMonitorRun() {}
	//MONITOR
	uint64 GetAcceptCount() const;
	uint64 GetAcceptTps() const;
	uint64 GetRecvTps() const;
	uint64 GetSendTps() const;
	int16 GetSessions() const;
	uint64 GetPacketPoolSize() const;
	uint32 GetPacketPoolEmptyCount() const;
	uint64 GetTimeoutCount() const;
	static uint32 GetPacketPoolAcquireCount();
	static uint32 GetPacketPoolReleaseCount();
	uint64 GetDisconnectCount() const;
	uint64 GetDisconnectPerSec() const;
	uint32 GetSegmentTimeout() const;

	size_t GetPageFaultCount() const;
	size_t GetPeakPMemSize() const;
	size_t GetPMemSize() const;
	size_t GetPeakVirtualMemSize() const;
	size_t GetVirtualMemSize() const;
	size_t GetPeakPagedPoolUsage() const;
	size_t GetPeakNonPagedPoolUsage() const;
	size_t GetPagedPoolUsage() const;
	size_t GetNonPagedPoolUsage() const;

	String GetLibMonitorString() const;
	void PrintMonitorString() const;

	template <typename GroupType, typename ...Args>
	GroupID CreateGroup(Args&&... args) const;
	void MoveSession(SessionID target, GroupID dst) const;
private:

	
	//MONITOR
	void IncreaseRecvCount(int value);
	void increaseSendCount(int value);

	void _onDisconnect(SessionID sessionId);
	void onRecvPacket(const Session& session, CRecvBuffer& buffer);

	//WorkerThreadFunc
	void WorkerThread(LPVOID arg);
	void AcceptThread(LPVOID arg);
	void MonitorThread(LPVOID arg);
	void TimeoutThread(LPVOID arg);
	void GroupThread(LPVOID arg);


	static unsigned __stdcall WorkerEntry(LPVOID arg);
	static unsigned __stdcall AcceptEntry(LPVOID arg);
	static unsigned __stdcall GroupEntry(const LPVOID arg);
	static unsigned __stdcall MonitorEntry(LPVOID arg);
	static unsigned __stdcall TimeoutEntry(LPVOID arg);


};


template <typename GroupType, typename ...Args>
inline GroupID IOCP::CreateGroup(Args&&... args) const
{
	return _groupManager->CreateGroup<GroupType>(std::forward<Args>(args)...);
}
