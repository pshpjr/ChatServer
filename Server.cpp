#include "stdafx.h"
#include "Server.h"
#include "ContentJob.h"
bool Server::OnAccept(SockAddr_in socket)
{
	return true;
}

void Server::OnConnect(SessionID sessionId)
{
	auto job = *ContentJob::Alloc();
	job._type = ContentJob::ePacketType::Connect;
	
	_packetQueue.Enqueue(job);
}

void Server::OnDisconnect(SessionID sessionId)
{
	auto job = *ContentJob::Alloc();
	job._type = ContentJob::ePacketType::Disconnect;

	_packetQueue.Enqueue(job);
}

void Server::OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer)
{
	auto job = *ContentJob::Alloc(&buffer);
	job._type = ContentJob::ePacketType::Packet;

	_packetQueue.Enqueue(job);
}
