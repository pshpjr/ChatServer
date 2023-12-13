#include "stdafx.h"
#include "RecvExecutable.h"
#include "Session.h"
#include "Protocol.h"
#include "CRecvBuffer.h"
#include "IOCP.h"

void RecvExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	PRO_BEGIN("RecvExecute");
	Session& session = *( Session* ) key;
	session.ioCount++;

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

template <typename Header>
void RecvExecutable::recvHandler(Session& session, void* iocp)
{

	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
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

		int totPacketSize = header->len + sizeof(Header);

		if ( recvQ.Size() < totPacketSize )
		{
			break;
		}

		Header* recvQHeader = ( Header* ) recvQ.GetFront();

		char* front;
		char* rear;

		int branch = 1;
		//if can pop direct
		if ( recvQ.DirectDequeueSize() >= header->len + sizeof(Header) )
		{
			branch = 2;
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

			if ( !buffer.checksumValid(header) )
			{
				//printf("checkSumInvalid %d %p\n", buffer._rear- buffer._front, buffer._front);

				session.Close();
				break;
			}
		}
		loopCount++;

		if ( session._groupID != 0 )
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
		server.increaseRecvCount(loopCount);

}