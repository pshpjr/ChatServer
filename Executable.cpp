#include "stdafx.h"
#include "Executable.h"

#include "Protocol.h"
#include "Session.h"
#include "IOCP.h"
#include "CSendBuffer.h"
#include "CRecvBuffer.h"


#include "CoreGlobal.h"
#include "CLogger.h"
#include "Profiler.h"


void SendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = ( Session* ) key;


	session->trySend();
}





void RecvExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	PRO_BEGIN("RecvExecute");
	Session& session = *( Session* ) key;

	session._recvQ.MoveRear(transferred);

	char sKey = session._staticKey;

	if ( sKey )
	{
		recvHandler<NetHeader>(session, iocp);
	}
	else
	{
		recvHandler<LANHeader>(session, iocp);
	}

	session.RecvNotIncrease();
}


void PostSendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = ( Session* ) key;
	auto& sendingQ = session->_sendingQ;

	CSendBuffer* buffer = nullptr;
	int sendingCount = session->dataNotSend;

	for ( int i = 0; sendingCount; i++ )
	{
		sendingQ[i]->Release(L"PostSendRelease");
	}

	session->dataNotSend = 0;

	session->needCheckSendTimeout = false;

	int oldSending = InterlockedExchange(&session->_isSending, 0);
	if ( oldSending != 1 )
	{
		DebugBreak();
	}

	InterlockedIncrement64(&( ( IOCP* ) iocp )->_sendCount);

	session->trySend();

	uint64 sessionID = session->_sessionID;
	session->Release(L"PostSendRelease");

}


void RecvExecutable::recvNormal(Session& session, void* iocp)
{
	using Header = LANHeader;
	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
	while ( true )
	{
		if ( session._recvQ.Size() < sizeof(Header) )
		{
			break;
		}

		Header header = *( Header* ) session._recvQ.GetFront();

		if ( session._recvQ.Size() < header.len )
		{
			break;
		}

		session._recvQ.Dequeue(sizeof(Header));

		auto front = session._recvQ.GetFront();
		auto rear = front + header.len;

		auto& buffer = *CRecvBuffer::Alloc(front, rear);
		session._recvQ.Dequeue(header.len);

		server.increaseRecvCount();

		try
		{
			( ( IOCP* ) iocp )->OnRecvPacket(session._sessionID, buffer);
		}
		catch ( const std::invalid_argument& )
		{
			GLogger->write(L"RecvErr", LogLevel::Debug, L"ERR");
		}

		buffer.Release(L"RecvRelease");

	}
}

void RecvExecutable::recvEncrypt(Session& session, void* iocp)
{
	//using Header = NetHeader;
	//IOCP& server = *reinterpret_cast< IOCP*>(iocp);
	//int loopCOunt = 0;

	//while (true)
	//{
	//	loopCOunt++;
	//	auto recvBuffer = session._recvBuffer;

	//	if ( recvBuffer->CanPopSize() < sizeof(Header))
	//	{
	//		break;
	//	}

	//	Header* header = ( Header* ) recvBuffer->GetFront();

	//	if ( header->code != dfPACKET_CODE )
	//	{
	//		session.Close();
	//		break;
	//	}

	//	if (header->len > session._maxPacketLen)
	//	{
	//		session.Close();
	//		break;
	//	}

	//	if (recvBuffer->CanPopSize() < header->len + sizeof(Header))
	//	{
	//		break;
	//	}
	//	
	//	auto& contentBuffer = *RecvBuffer::Alloc(recvBuffer->GetFront(), recvBuffer->GetRear());
	//	contentBuffer.isEncrypt++;
	//	contentBuffer.Decode(session._staticKey, ( Header* ) contentBuffer.GetFront());

	//	if (!contentBuffer.checksumValid(( Header* ) contentBuffer.GetFront()))
	//	{
	//		//printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

	//		session.Close();
	//		break;
	//	}
	//	contentBuffer.MoveReadPos(sizeof(Header));

	//	server.increaseRecvCount();
	//	//session.debugIndex++;
	//	//session.Debug[session.debugIndex] = { packetBegin,session._recvQ.GetFront(),session._recvQ.GetRear() };

	//	try
	//	{

	//		((IOCP*)iocp)->OnRecvPacket(session._sessionID, contentBuffer);
	//	}
	//	catch (const std::invalid_argument& e)
	//	{

	//		printf("%s",e.what());
	//		session.Close();
	//	}

	//	recvBuffer->MoveReadPos(sizeof(Header));
	//	recvBuffer->MoveReadPos(header->len);
	//	contentBuffer.Release(L"RecvRelease");
	//}
}

template <typename Header>
void RecvExecutable::recvHandler(Session& session, void* iocp)
{
	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
	while ( true )
	{
		if ( session._recvQ.Size() < sizeof(Header) )
		{
			break;
		}

		Header* header = ( Header* ) session._recvQ.GetFront();

		if constexpr ( is_same_v<Header, NetHeader> )
		{
			if ( header->code != dfPACKET_CODE )
			{
				session.Close();
				break;
			}

			if ( header->len > session._maxPacketLen )
			{
				session.Close();
				break;
			}
		}

		if ( session._recvQ.Size() < header->len )
		{
			break;
		}

		session._recvQ.Dequeue(sizeof(Header));

		char* front = session._recvQ.GetFront();
		char* rear = front + header->len;

		auto& buffer = *CRecvBuffer::Alloc(front, rear);

		if constexpr ( is_same_v<Header, NetHeader> )
		{
			buffer.Decode(session._staticKey, header);

			if ( !buffer.checksumValid(header) )
			{
				//printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

				session.Close();
				break;
			}
		}

		server.increaseRecvCount();
		try
		{
			( ( IOCP* ) iocp )->OnRecvPacket(session._sessionID, buffer);
		}
		catch ( const std::invalid_argument& )
		{
			GLogger->write(L"RecvErr", LogLevel::Debug, L"ERR");
		}

		session._recvQ.Dequeue(header->len);

		buffer.Release(L"RecvRelease");
	}
}

void ReleaseExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = ( Session* ) key;

	session->Reset();

	session->_owner->_onDisconnect(session->_sessionID);
	unsigned short index = ( uint16 ) ( session->_sessionID >> 47 );
	session->_owner->freeIndex.Push(index);
}