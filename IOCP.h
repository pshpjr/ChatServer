#pragma once
#include "BuildOption.h"
#include "types.h"

class CSendbuffer;
class CRecvBuffer;

class IOCP : public IOCP_CLASS
{
	friend class RecvExecutable;
	friend class PostSendExecutable;
	friend class ReleaseExecutable;
	friend class Session;
public:
	IOCP();

	bool Init(String ip, Port port, uint16 backlog, uint16 maxRunningThread,uint16 workerThread, char staticKey);
	void Start();
	void Stop();
	inline bool SendPacket(SessionID sessionId, CSendBuffer* buffer, int type);
	bool SendPacket(SessionID sessionId, CSendBuffer* buffer);
	bool SendPackets(SessionID sessionId, list<CSendBuffer*>& bufArr);
	bool DisconnectSession(SessionID sessionId);
	bool isEnd() const;
	void SetMaxPacketSize(int size);
	void SetTimeout(SessionID sessionId, int timeoutMillisecond);
	void SetDefaultTimeout(unsigned int timeoutMillisecond);
	void PostExecutable(Executable* exe, ULONG_PTR arg);
	

	void PrintLibMonitorInfo() const;
	virtual void OnWorkerThreadBegin() {}; 
	virtual void OnWorkerThreadEnd() {};
	virtual bool OnAccept(SockAddr_in) { return true; };
	virtual void OnConnect(SessionID sessionId, const SockAddr_in& info) {};
	virtual void OnDisconnect(SessionID sessionId) {};
	virtual void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) {};
	virtual void OnInit() {};
	virtual void OnStart() {};
	virtual void OnEnd() {};
	virtual void OnSessionTimeout(SessionID sessionId,String ip, Port port) {};
	virtual void OnMonitorRun() {};

	//MONITOR
	uint64 GetAcceptCount() const;
	uint64 GetAcceptTps() const;
	uint64 GetRecvTps() const;
	uint64 GetSendTps() const;
	uint16 GetSessions() const;
	uint64 GetPacketPoolSize() const;
	uint32 GetPacketPoolEmptyCount() const;
	uint64 GetTimeoutCount() const;
	uint32 GetPacketPoolAcquireCount() const;
	uint32 GetPacketPoolReleaseCount() const;
	uint64 GetDisconnectCount() const;
	uint64 GetDisconnectPersec() const;
	uint32 GetSegmentTimeout() const;

	size_t GetPageFaultCount() const;
	size_t GetPeakPMemSize() const;
	size_t GetPMemSize() const;
	size_t GetPeakVmemSize() const;
	size_t GetVMemsize() const;
	size_t GetPeakPagedPoolUsage() const;
	size_t GetPeakNonPagedPoolUsage() const;
	size_t GetPagedPoolUsage() const;
	size_t GetNonPagedPoolUsage() const;


private:
	//MONITOR
	void increaseRecvCount();

	void _onDisconnect(SessionID sessionId);

	//WorkerThreadFunc
	void WorkerThread(LPVOID arg);
	void AcceptThread(LPVOID arg);
	void MonitorThread(LPVOID arg);
	void TimeoutThread(LPVOID arg);



	static unsigned __stdcall WorkerEntry(LPVOID arg);
	static unsigned __stdcall AcceptEntry(LPVOID arg);
	static unsigned __stdcall MonitorEntry(LPVOID arg);
	static unsigned __stdcall TimeoutEntry(LPVOID arg);


};

