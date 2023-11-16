#pragma once
#include "CSerializeBuffer.h"


class IOCP : public IOCP_CLASS
{
	friend class RecvExecutable;
	friend class PostSendExecutable;
	friend class ReleaseExecutable;
	friend class Session;
public:
	IOCP();

	using SessionID = uint64;
	using Port = uint16;

	bool Init(String ip, Port port, uint16 backlog, uint16 maxRunningThread,uint16 workerThread, char staticKey);
	void Start();
	void Stop();
	bool SendPacket(SessionID sessionId, CSerializeBuffer* buffer, int type);
	bool SendPacket(SessionID sessionId, CSerializeBuffer* buffer);
	bool DisconnectSession(SessionID sessionId);
	bool isEnd();
	void SetMaxPacketSize(int size);
	void SetTimeout(SessionID sessionId, int timeoutMillisecond);

	virtual void OnWorkerThreadBegin() {}; 
	virtual void OnWorkerThreadEnd() {};
	virtual bool OnAccept(SockAddr_in) { return true; };
	virtual void OnConnect(SessionID sessionId) {};
	virtual void OnDisconnect(SessionID sessionId) {};
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer) {};
	virtual void OnInit() {};
	virtual void OnStart() {};
	virtual void OnEnd() {};
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
	uint64 GetTimeoutDisconnectSession();

private:
	void onDisconnect(SessionID sessionId);
	void WorkerThread(LPVOID arg);
	void AcceptThread(LPVOID arg);
	void MonitorThread(LPVOID arg);
	void TimeoutThread(LPVOID arg);

	static unsigned __stdcall WorkerEntry(LPVOID arg);
	static unsigned __stdcall AcceptEntry(LPVOID arg);
	static unsigned __stdcall MonitorEntry(LPVOID arg);
	static unsigned __stdcall TimeoutEntry(LPVOID arg);


};

