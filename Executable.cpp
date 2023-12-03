#include "stdafx.h"
#include "Executable.h"

#include "Protocol.h"
#include "Session.h"
#include "IOCP.h"
#include "CSerializeBuffer.h"
#include "CoreGlobal.h"
#include "Profiler.h"

void RecvExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	Session& session = *(Session*)key;

	session._recvBuffer->MoveWritePos(transferred);

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
	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
	while (true)
	{
		if (session._recvQ.Size() < sizeof(Header))
		{

			break;
		}

		Header header {};
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

		server.increaseRecvCount();


		try
		{
			((IOCP*)iocp)->OnRecvPacket(session._sessionID, buffer);
		}
		catch (const std::invalid_argument&)
		{
			GLogger->write(L"RecvErr", LogLevel::Debug, L"ERR");
		}

		buffer.Release(L"RecvRelease");

	}
}

void RecvExecutable::recvEncrypt(Session& session, void* iocp)
{
	using Header = CSerializeBuffer::NetHeader;
	IOCP& server = *reinterpret_cast< IOCP*>(iocp);
	int loopCOunt = 0;

	while (true)
	{
		loopCOunt++;
		auto recvBuffer = session._recvBuffer;

		if ( recvBuffer->GetPacketSize() < sizeof(Header))
		{
			break;
		}

		Header* header = ( Header* ) recvBuffer->GetFront();

		if (header->len > session._maxPacketLen)
		{
			session.Close();
			break;
		}

		if (header->code != dfPACKET_CODE)
		{
			session.Close();
			break;
		}

		if (recvBuffer->GetDataSize() < header->len + sizeof(Header))
		{
			break;
		}
		
		auto& contentBuffer = *ContentSerializeBuffer::Alloc(recvBuffer->GetFront(), recvBuffer->GetRear());
		contentBuffer.isEncrypt++;
		contentBuffer.Decode(session._staticKey, ( Header* ) contentBuffer.GetFront());

		if (!contentBuffer.checksumValid(( Header* ) contentBuffer.GetFront()))
		{
			//printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

			session.Close();
			break;
		}
		contentBuffer.MoveReadPos(sizeof(Header));

		server.increaseRecvCount();
		//session.debugIndex++;
		//session.Debug[session.debugIndex] = { packetBegin,session._recvQ.GetFront(),session._recvQ.GetRear() };

		try
		{
			((IOCP*)iocp)->OnRecvPacket(session._sessionID, contentBuffer);
		}
		catch (const std::invalid_argument& e)
		{

			printf("%s",e.what());
			session.Close();
		}

		recvBuffer->MoveReadPos(sizeof(Header));
		recvBuffer->MoveReadPos(header->len);
		contentBuffer.Release(L"RecvRelease");
	}
}


void PostSendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;
	auto& sendingQ = session->_sendingQ;

	CSerializeBuffer* buffer = nullptr;


	while (sendingQ.Dequeue(buffer))
	{
		buffer->Release(L"PostSendRelease");
	}


	session->dataNotSend = 0;

	session->needCheckSendTimeout = false;
	
	int oldSending = InterlockedExchange(&session->_isSending, 0);
	if (oldSending != 1) {
		DebugBreak();
	}

	InterlockedIncrement64(&((IOCP*)iocp)->_sendCount);
	
	session->trySend();

	uint64 sessionID = session->_sessionID;
	session->Release(L"PostSendRelease");

}

void SendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;


	session->trySend();
}

void ReleaseExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	Session* session = (Session*)key;

	session->Reset();

	session->_owner->_onDisconnect(session->_sessionID);
	unsigned short index = (uint16) (session->_sessionID >> 47);
	session->_owner->freeIndex.Push(index);
}
