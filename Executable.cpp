#include "stdafx.h"
#include "Executable.h"

#include "Protocol.h"
#include "Session.h"
#include "IOCP.h"
#include "CSerializeBuffer.h"


void RecvExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = (Session*)key;

	session->_recvQ.MoveRear(transferred);

	while (true)
	{
		if (session->_recvQ.Size() < sizeof(Header))
		{

			break;
		}

		Header header;
		session->_recvQ.Peek((char*)&header, sizeof(Header));


		if (session->_recvQ.Size() < header.len)
		{

			break;
		}


		session->_recvQ.Dequeue(sizeof(Header));

		auto& buffer = *CSerializeBuffer::Alloc();
		session->_recvQ.Peek(buffer.GetBufferPtr(), header.len);
		session->_recvQ.Dequeue(header.len);
		buffer.MoveWritePos(header.len);
		InterlockedIncrement(&((IOCP*)iocp)->_recvCount);

		((IOCP*)iocp)->OnRecvPacket(session->_sessionID, buffer);
		buffer.Release();

	}
	session->registerRecv();


	uint64 sessionID = session->_sessionID;
	if (session->Release())
	{
		((IOCP*)iocp)->OnDisconnect(sessionID);
	}
}

void PostSendExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = (Session*)key;

	session->_sendQ.DequeueCBuffer(session->sendingSerializeBuffers);
	session->dataNotSend = 0;
	isSend = false;
	int oldSending = InterlockedExchange(&session->_isSending, 0);
	InterlockedIncrement(&((IOCP*)iocp)->_sendCount);
	
	session->trySend();

	uint64 sessionID = session->_sessionID;
	if (session->Release())
	{
		((IOCP*)iocp)->OnDisconnect(sessionID);
	}
}

void SendExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;


	session->trySend();
}
