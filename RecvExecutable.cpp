#include "stdafx.h"
#include "RecvExecutable.h"
#include "Session.h"
#include "Protocol.h"
#include "CRecvBuffer.h"
#include "IOCP.h"

void RecvExecutable::Execute(const ULONG_PTR key, const DWORD transferred, void* iocp)
{
	Session& session = *reinterpret_cast<Session*>(key);
	session.ioCount++;

	session._recvQ.MoveRear(transferred);

	if ( const char sKey = session._staticKey )
	{
		RecvHandler<NetHeader>(session, iocp);
	}
	else
	{
		RecvHandler<LANHeader>(session, iocp);
	}

	session.RecvNotIncrease();
}

template <typename Header>
void RecvExecutable::RecvHandler(Session& session, void* iocp)
{

	IOCP& server = *static_cast< IOCP* >( iocp );
	int loopCount = 0;

	CRingBuffer& recvQ = session._recvQ;
	while ( true )
	{
		if ( recvQ.Size() < sizeof(Header) )
		{
			break;
		}


		Header* header = ( Header* ) session._recvTempBuffer;
		recvQ.Peek(( char* ) header, sizeof(Header));

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

		if (const int totPacketSize = header->len + sizeof(Header); recvQ.Size() < totPacketSize )
		{
			break;
		}

		Header* recvQHeader = ( Header* ) recvQ.GetFront();

		char* front;
		char* rear;
		
		//if can pop direct
		if ( recvQ.DirectDequeueSize() >= header->len + sizeof(Header) )
		{
			recvQ.Dequeue(sizeof(Header));
			front = session._recvQ.GetFront();
			rear = front + header->len;
			header = recvQHeader;
		}
		else
		{
			front = ( char* ) header + sizeof(Header);
			recvQ.Dequeue(sizeof(Header));
			recvQ.Peek(front, header->len);
			rear = front + header->len;
		}


		auto& buffer = *CRecvBuffer::Alloc(front, rear);

		if constexpr ( is_same_v<Header, NetHeader> )
		{
			buffer.Decode(session._staticKey, header);

			if ( !buffer.ChecksumValid(header) )
			{
				//printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

				session.Close();
				break;
			}
		}
		loopCount++;

		if ( session._groupId != 0 )
		{
			session._groupRecvQ.Enqueue(&buffer);
		}
		else
		{
			server.onRecvPacket(session, buffer);

			buffer.Release(L"RecvRelease");
		}
		session._recvQ.Dequeue(header->len);
	}

	if ( loopCount > 0 )
	{
		server.IncreaseRecvCount(loopCount);
	}
}
