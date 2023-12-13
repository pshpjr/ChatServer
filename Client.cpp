#include "stdafx.h"
#include "Client.h"
#include "Group.h"
#include "GroupManager.h"
#include "IOCP.h"


void Client::Send(CSendBuffer& buffer)
{
	_iocp->SendPacket(_id, &buffer);
}

void Client::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
{
	onRecv(recvBuffer);
}
