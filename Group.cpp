#include "stdafx.h"
#include "Group.h"
#include "IOCP.h"
#include "CRecvBuffer.h"
#include "GroupExecutable.h"
#include "GroupManager.h"
#include <Protocol.h>
#include <CLogger.h>
#include "CoreGlobal.h"
Group::Group() :_executable(*new GroupExecutable(this)),_iocp(nullptr),_owner(nullptr)
,_nextUpdate(std::chrono::steady_clock::now() + chrono::milliseconds(_loopMs)) {};

bool Group::NeedUpdate()
{
	return ::chrono::steady_clock::now() >= _nextUpdate;
}

void Group::Update()
{
	if (InterlockedExchange8(&_isRun, 1) == 1)
		return;
	
	HandleEnter();
	HandlePacket();
	if (NeedUpdate()) 
	{
		OnUpdate();
		_nextUpdate += chrono::milliseconds(_loopMs);
	}
	
	HandleLeave();

	InterlockedExchange8(&_isRun, 0);
}

void Group::HandleEnter()
{
	SessionID id;
	int count = 0;
	while ( _enterSessions.Dequeue(id) )
	{
		++count;
		_sessions.insert(id);
		OnEnter(id);
		if(count == 100)
			break;
	}
}
void Group::HandleLeave()
{
	SessionID id;
	int count = 0;
	while ( _leaveSessions.Dequeue(id) )
	{
		++count;
		if ( _sessions.erase(id) == 0 )
		{
			DebugBreak();
		}
		
		OnLeave(id);

		if(_iocp->isRelease(id))
		{
			_iocp->postReleaseSession(id);
		}
		if(count == 100)
			break;
	}
}
//onDisconnect랑 OnSessionLeave랑 순서가 안 맞을 수 있다. 
void Group::HandlePacket()
{
	for (const auto sessionId : _sessions )
	{
		auto session = _iocp->FindSession(sessionId,L"HandlePacket");
		if ( session == nullptr )
		{
			LeaveSession(sessionId);
			continue;
		}

		if (const char sKey = session->_staticKey)
		{
			RecvHandler<NetHeader>(*session, _iocp);
		}
		else
		{
			RecvHandler<LANHeader>(*session, _iocp);
		}
		
		session->Release(L"HandlePacketRelease");
	}
}

void Group::onRecvPacket(const Session& session, CRecvBuffer& buffer)
{
	try
	{
		OnRecv(session._sessionId, buffer);
		ASSERT_CRASH(buffer.CanPopSize() == 0, "can pop from recv buffer");
	}
	catch (const std::invalid_argument&)
	{
		_iocp->DisconnectSession(session.GetSessionId());
		gLogger->Write(L"Recv Pop Err", LogLevel::Debug, L"ERR");
	}
}


//해당 세션이 언제 나갈지 모르는데 상관 없나?
//어짜피 동시에 처리되는 건 아니니까 상관 x.
void Group::LeaveSession(const SessionID id)
{
	_leaveSessions.Enqueue(id);
}

void Group::EnterSession(const SessionID id)
{
	_enterSessions.Enqueue(id);
}

void Group::SetLoopMs(int loopMS)
{
	_loopMs = loopMS;
}

void Group::SendPacket(const SessionID id, SendBuffer& buffer) const
{
	_iocp->SendPacket(id, buffer);
}

void Group::MoveSession(const SessionID id, const GroupID dst) const
{
	_owner->MoveSession(id, dst);
}

void Group::Execute(IOCP* iocp) const
{
	_executable.Execute(0, executable::ExecutableTransfer::GROUP, iocp);
}




template <typename Header>
void Group::RecvHandler(Session& session, void* iocp)
{

	IOCP& server = *static_cast<IOCP*>(iocp);
	int loopCount = 0;

	CRingBuffer& recvQ = session._recvQ;
	while (true)
	{
		if (session.GetGroupID() != _groupId)
		{
			LeaveSession(session.GetSessionId());
			break;
		}

		if (recvQ.Size() < sizeof(Header))
		{
			break;
		}


		Header header;
		recvQ.Peek((char*)&header, sizeof(Header));

		if constexpr (is_same_v<Header, NetHeader>)
		{
			if (header.code != dfPACKET_CODE)
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				SessionID id = session.GetSessionId();
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"Group WrongPacketCode, SessionID : %d\n frontIndex: %d \n %s ",id, frontIndex, dump);
				session.Close();
				break;
			}

			if (header.len > session._maxPacketLen)
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				SessionID id = session.GetSessionId();
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"Group WrongPacketLen, SessionID : %d\n index: %d \n %s",id, frontIndex, dump);
				session.Close();
				break;
			}
		}

		if (const int totPacketSize = header.len + sizeof(Header); recvQ.Size() < totPacketSize)
		{
			break;
		}

		auto& buffer = *CRecvBuffer::Alloc(&recvQ, header.len);


		if constexpr (is_same_v<Header, NetHeader>)
		{
			recvQ.Decode(session._staticKey);

			if (!recvQ.ChecksumValid())
			{
				
				gLogger->Write(L"Disconnect", LogLevel::Debug, L"Group Invalid Checksum %d",session.GetSessionId());
				session.Close();
				break;
			}
		}
		loopCount++;
		recvQ.Dequeue(sizeof(Header));
		onRecvPacket(session, buffer);



		buffer.Release(L"RecvRelease");
	}

	if (loopCount > 0)
	{
		server.IncreaseRecvCount(loopCount);
	}
}
