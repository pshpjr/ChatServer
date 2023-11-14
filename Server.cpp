#include "stdafx.h"
#include "Server.h"
#include "ContentJob.h"
#include "Player.h"
#include <thread>

bool Server::OnAccept(SockAddr_in socket)
{
	return true;
}

void Server::OnConnect(SessionID sessionId)
{
	auto job = ContentJob::Alloc();
	job->_type = ContentJob::ePacketType::Connect;
	job->_id = sessionId;
	
	_packetQueue.Enqueue(job);
}

void Server::OnDisconnect(SessionID sessionId)
{
	auto job = ContentJob::Alloc();
	job->_type = ContentJob::ePacketType::Disconnect;
	job->_id = sessionId;

	_packetQueue.Enqueue(job);
}

void Server::OnRecvPacket(SessionID sessionId, CSerializeBuffer& buffer)
{
	auto job = ContentJob::Alloc(&buffer);
	job->_type = ContentJob::ePacketType::Packet;
	job->_id = sessionId;

	_packetQueue.Enqueue(job);
}



void Server::OnStart()
{
	timeoutThread = std::thread(&Server::TimeoutCheck, this);
}
void Server::OnEnd()
{
	if (timeoutThread.joinable())
		timeoutThread.join();
}

void Server::TimeoutCheck()
{
	auto nextWakeup = std::chrono::system_clock::now();
	while (_isRunning)
	{
		auto job = ContentJob::Alloc();
		job->_type = ContentJob::ePacketType::TimeoutCheck;
		job->_buffer = nullptr;
		_packetQueue.Enqueue(job);

		auto sleepTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextWakeup - std::chrono::system_clock::now()).count();
		nextWakeup += std::chrono::seconds(1);
		Sleep(sleepTime);
	}
}


