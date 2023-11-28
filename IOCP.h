#pragma once
#include "CSerializeBuffer.h"
#include "types.h"

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
	bool SendPacket(SessionID sessionId, CSerializeBuffer* buffer, int type);
	bool SendPacket(SessionID sessionId, CSerializeBuffer* buffer);
	bool SendPackets(SessionID sessionId, list<CSerializeBuffer*>& bufArr);
	bool DisconnectSession(SessionID sessionId);
	bool isEnd();
	void SetMaxPacketSize(int size);
	void SetTimeout(SessionID sessionId, int timeoutMillisecond);
	void SetDefaultTimeout(unsigned int timeoutMillisecond);
	void PostExecutable(Executable* exe, ULONG_PTR arg);


	virtual void OnWorkerThreadBegin() {}; 
	virtual void OnWorkerThreadEnd() {};
	virtual bool OnAccept(SockAddr_in) { return true; };
	virtual void OnConnect(SessionID sessionId, const SockAddr_in& info) {};
	virtual void OnDisconnect(SessionID sessionId) {};
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer) {};
	virtual void OnInit() {};
	virtual void OnStart() {};
	virtual void OnEnd() {};
	virtual void OnSessionTimeout(SessionID sessionId,String ip, Port port) {};


	//DEBUG
	void SetRecvDebug(SessionID id, unsigned int type);


	//MONITOR


	uint64 GetAcceptCount();
	uint64 GetAcceptTps();
	uint64 GetRecvTps();
	uint64 GetSendTps();
	uint16 GetSessions();
	uint64 GetPacketPoolSize();
	uint32 GetPacketPoolEmptyCount();
	uint64 GetTimeoutCount();
	uint32 GetPacketPoolAcquireCount();
	uint32 GetPacketPoolReleaseCount();


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

