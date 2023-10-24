#include "stdafx.h"
#include "Server.h"

bool Server::OnAccept(SockAddr_in socket)
{
	return true;
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
