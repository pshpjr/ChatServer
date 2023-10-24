#pragma once
#include "IOCP.h"
#pragma comment(lib, "winmm.lib")


class Server : public IOCP
{
	
	bool OnAccept(Socket socket);
	virtual void OnConnect(SessionID sessionId);
	virtual void OnDisconnect(SessionID sessionId);
	virtual void OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer);

	
};

