#pragma once
#include "IOCP.h"
#pragma comment(lib, "winmm.lib")


class Server : public IOCP
{
	
	bool OnAccept(SockAddr_in socket) override;
	virtual void OnConnect(SessionID sessionId) override;
	virtual void OnDisconnect(SessionID sessionId) override;
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer) override;

	
};

