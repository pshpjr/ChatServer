#include "stdafx.h"
#include "Executable.h"

#include "Protocol.h"
#include "Session.h"
#include "IOCP.h"
#include "CSerializeBuffer.h"


void RecvExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{
	Session& session = *(Session*)key;

	session._recvQ.MoveRear(transferred);

	char sKey = session._staticKey;

	if (sKey) 
	{
		recvEncrypt(session,iocp);
	}
	else 
	{
		recvNormal(session,iocp);
	}


	session.RecvNotIncrease();
}

void RecvExecutable::recvNormal(Session& session, void* iocp)
{
	using Header = CSerializeBuffer::LANHeader;

	while (true)
	{
		if (session._recvQ.Size() < sizeof(Header))
		{

			break;
		}

		Header header;
		session._recvQ.Peek((char*)&header, sizeof(Header));

		if (session._recvQ.Size() < header.len)
		{
			break;
		}

		session._recvQ.Dequeue(sizeof(Header));

		auto& buffer = *CSerializeBuffer::Alloc();
		session._recvQ.Peek(buffer.GetDataPtr(), header.len);
		session._recvQ.Dequeue(header.len);
		buffer.MoveWritePos(header.len);
		InterlockedIncrement(&((IOCP*)iocp)->_recvCount);

		try
		{
			((IOCP*)iocp)->OnRecvPacket(session._sessionID, buffer);
		}
		catch (const std::invalid_argument& err)
		{
			printf("ERR");
		}
		buffer.Release();

	}
}

void RecvExecutable::recvEncrypt(Session& session, void* iocp)
{
	using Header = CSerializeBuffer::NetHeader;
	int loopCOunt = 0;

	while (true)
	{
		loopCOunt++;
		if (session._recvQ.Size() < sizeof(Header))
		{

			break;
		}

		Header header;

		char* packetBegin = nullptr;
		session._recvQ.Peek((char*)&header, sizeof(Header));
		packetBegin = session._recvQ.GetFront();
		if (header.code != dfPACKET_CODE) 
		{
			session.Close();
			break;
		}

		if (session._recvQ.Size() < header.len + sizeof(Header))
		{
			break;
		}

		session._recvQ.Dequeue(sizeof(Header));

		auto& buffer = *CSerializeBuffer::Alloc();

		session._recvQ.Peek(buffer.GetDataPtr(), header.len);
		session._recvQ.Dequeue(header.len);

		buffer.MoveWritePos(header.len);
		buffer.setEncryptHeader(header);
		buffer.isEncrypt++;

		buffer.Decode(session._staticKey);
		if (!buffer.checksumValid()) 
		{
			printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

			session.Close();
			break;
		}

		InterlockedIncrement(&((IOCP*)iocp)->_recvCount);
		//session.debugIndex++;
		//session.Debug[session.debugIndex] = { packetBegin,session._recvQ.GetFront(),session._recvQ.GetRear() };

		try
		{
			((IOCP*)iocp)->OnRecvPacket(session._sessionID, buffer);
		}
		catch (const std::invalid_argument& err)
		{
			session.Close();
		}


		buffer.Release();
	}
}

int compCount;
void PostSendExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;
	auto& sendingQ = session->_sendingQ;

	CSerializeBuffer* buffer = nullptr;


	while (sendingQ.Dequeue(buffer))
	{
		buffer->Release();
	}


	session->dataNotSend = 0;

	session->needCheckSendTimeout = false;
	
	int oldSending = InterlockedExchange(&session->_isSending, 0);
	if (oldSending != 1) {
		DebugBreak();
	}

	InterlockedIncrement(&((IOCP*)iocp)->_sendCount);
	
	session->trySend();

	uint64 sessionID = session->_sessionID;
	session->Release(L"PostSendRelease");

}

void SendExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;


	session->trySend();
}

void ReleaseExecutable::Execute(PULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;

	session->Reset();

	session->_owner->onDisconnect(session->_sessionID);
	auto index = session->_sessionID >> 47;
	session->_owner->freeIndex.Push(index);
}
