#pragma once
#include "IOCP.h"
#pragma comment(lib, "winmm.lib")
#include "PacketQueue.h"

class Server : public IOCP
{
public:
	bool OnAccept(SockAddr_in socket) override;
	virtual void OnConnect(SessionID sessionId) override;
	virtual void OnDisconnect(SessionID sessionId) override;
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer) override;
	virtual void OnStart() override;
	

	PacketQueue _packetQueue;
};

