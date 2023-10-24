#pragma once
#include "CSerializeBuffer.h"


class IOCP : public IOCP_CLASS
{
public:

	bool Init(String ip, uint16 port, uint16 backlog, uint16 maxNetThread);
	void Start();
	void Stop();
	bool SendPacket(uint64 sessionId, CSerializeBuffer* buffer);
	bool DisconnectSession(uint64 sessionId);
	virtual void OnWorkerThreadBegin() {}; 
	virtual void OnWorkerThreadEnd() {};
	virtual bool OnAccept(Socket socket) { return true; };
	virtual void OnConnect(uint64 sessionId) {};
	virtual void OnDisconnect(uint64 sessionId) {};
	virtual void OnRecvPacket(uint64 sessionId, CSerializeBuffer& buffer) {};

	int64 GetAcceptTps();
	int64 GetRecvTps();
	int64 GetSendTps();

private:
	void WorkerThread(LPVOID arg);
	void AcceptThread(LPVOID arg);
	void MonitorThread(LPVOID arg);

	static unsigned __stdcall WorkerEntry(LPVOID arg);
	static unsigned __stdcall AcceptEntry(LPVOID arg);
	static unsigned __stdcall MonitorEntry(LPVOID arg);


};

