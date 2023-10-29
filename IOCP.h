#pragma once
#include "CSerializeBuffer.h"


class IOCP : public IOCP_CLASS
{


public:
	IOCP();

	using SessionID = uint64;
	using Port = uint16;


	bool Init(String ip, Port port, uint16 backlog, uint16 maxNetThread);
	void Start();
	void Stop();
	bool SendPacket(SessionID sessionId, CSerializeBuffer* buffer);
	bool DisconnectSession(SessionID sessionId);
	virtual void OnWorkerThreadBegin() {}; 
	virtual void OnWorkerThreadEnd() {};
	virtual bool OnAccept(SockAddr_in) { return true; };
	virtual void OnConnect(SessionID sessionId) {};
	virtual void OnDisconnect(SessionID sessionId) {};
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer) {};
	virtual void OnInit() {};
	virtual void OnStart() {};
	virtual void OnEnd() {};

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

