#include "stdafx.h"
#include "Group.h"
#include "IOCP.h"
#include "CRecvBuffer.h"
#include "GroupExecutable.h"
#include "GroupManager.h"

Group::Group() :_executable(*new GroupExecutable(this)),_iocp(nullptr),_owner(nullptr) {};

void Group::Update()
{
	HandleEnter();
	HandlePacket();
	OnUpdate();
	HandleLeave();
}

void Group::HandleEnter()
{
	SessionID id;


	while ( _enterSessions.Dequeue(id) )
	{
		_sessions.insert(id);
		OnEnter(id);
	}
}
void Group::HandleLeave()
{
	SessionID id;
	while ( _leaveSessions.Dequeue(id) )
	{
		if ( _sessions.erase(id) == 0 )
		{
			DebugBreak();
		}

		OnLeave(id);
	}
}
//onDisconnect랑 OnSessionLeave랑 순서가 안 맞을 수 있다. 
void Group::HandlePacket()
{
	for (const auto sessionId : _sessions )
	{
		const auto session = _iocp->FindSession(sessionId,L"HandlePacket");
		if ( session == nullptr )
		{
			LeaveSession(sessionId);
			continue;
		}
		
		while ( true )
		{
			if ( session->GetGroupID() != _groupId )
			{
				LeaveSession(sessionId);
				break;
			}

			CRecvBuffer* recvBuffer;
			if ( !session->_groupRecvQ.Dequeue(recvBuffer) )
			{
				break;
			}

			OnRecv(sessionId, *recvBuffer);

			recvBuffer->Release(L"GroupRecvRelease");
		}



		session->Release(L"HandlePacketRelease");
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

void Group::SendPacket(const SessionID id, CSendBuffer& buffer) const
{
	_iocp->SendPacket(id, &buffer);
}

void Group::MoveSession(const SessionID id, const GroupID dst) const
{
	_owner->MoveSession(id, dst);
}

void Group::Execute(IOCP* iocp) const
{
	_executable.Execute(0, executable::ExecutableTransfer::GROUP, iocp);
}


