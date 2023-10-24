#include "stdafx.h"
#include "Server.h"

bool Server::OnAccept(Socket socket)
{
	return false;
}

void Server::OnConnect(SessionID sessionId)
{
}

void Server::OnDisconnect(SessionID sessionId)
{
}

void Server::OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer)
{
}
