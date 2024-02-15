#include "stdafx.h"
#include "RecvExecutable.h"
#include "Session.h"
#include "Protocol.h"
#include "CRecvBuffer.h"
#include "CoreGlobal.h"
#include "CLogger.h"
#include "IOCP.h"

void RecvExecutable::Execute(const ULONG_PTR key, const DWORD transferred, void* iocp)
{
	Session& session = *reinterpret_cast<Session*>(key);
	session.ioCount++;

	session._recvQ.MoveRear(transferred);

	if (session.GetGroupID() != 0) 
	{

	}
	else if ( const char sKey = session._staticKey )
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


		Header header;
		recvQ.Peek(( char* )&header, sizeof(Header));

		if constexpr ( is_same_v<Header, NetHeader> )
		{
			if ( header.code != dfPACKET_CODE )
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"WrongPacketCode id : %d\n index: %d \n %s", session.GetSessionId(),frontIndex,dump);
				session.Close();
				break;
			}

			if ( header.len > session._maxPacketLen )
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"WrongPacketLen id : %d\n index: %d \n %s", session.GetSessionId(), frontIndex, dump);
				session.Close();
				break;
			}
		}

		if(header.len > recvQ.Size())
			break;
		
		
		if (const int totPacketSize = header.len + sizeof(Header); recvQ.Size() < totPacketSize )
		{
			break;
		}

		auto& buffer = *CRecvBuffer::Alloc(&recvQ,header.len);

		if constexpr ( is_same_v<Header, NetHeader> )
		{
			recvQ.Decode(session._staticKey);

			if (!recvQ.ChecksumValid() )
			{
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"Recv Invalid Checksum id : %d",session.GetSessionId());
				session.Close();
				break;
			}
		}
		loopCount++;
		recvQ.Dequeue(sizeof(Header));
		server.onRecvPacket(session, buffer);

		ASSERT_CRASH(buffer.CanPopSize() == 0,"UnuseData");
		buffer.Release(L"RecvRelease");
		
	}

	if ( loopCount > 0 )
	{
		server.IncreaseRecvCount(loopCount);
	}
}
