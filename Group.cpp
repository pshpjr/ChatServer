#include "stdafx.h"
#include "Group.h"
#include "IOCP.h"
#include "CRecvBuffer.h"
#include "GroupExecutable.h"
#include "GroupManager.h"

Group::Group() :_executable(*new GroupExecutable(this)), _enterSessions(), _leaveSessions(),_owner(nullptr),_iocp(nullptr) {};

void Group::Update()
{
	handleEnter();
	handlePacket();
	OnUpdate();
	handleLeave();
}

void Group::handleEnter()
{
	SessionID id;


	while ( _enterSessions.Dequeue(id) )
	{
		_sessions.insert(id);
		OnEnter(id);
	}
}
void Group::handleLeave()
{
	SessionID id;
	while ( _leaveSessions.Dequeue(id) )
	{
		if ( _sessions.erase(id) == 0 )
			DebugBreak();

		OnLeave(id);
	}
}
//onDisconnect랑 OnSessionLeave랑 순서가 안 맞을 수 있다. 
void Group::handlePacket()
{
	for ( auto sessionId : _sessions )
	{
		auto session = _iocp->FindSession(sessionId,L"HandlePacket");
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
void Group::LeaveSession(SessionID id)
{
	_leaveSessions.Enqueue(id);
}

void Group::EnterSession(SessionID id)
{
	_enterSessions.Enqueue(id);
}

void Group::SendPacket(SessionID id, CSendBuffer& buffer)
{
	_iocp->SendPacket(id, &buffer);
}

void Group::MoveSession(SessionID id, GroupID dst)
{
	_owner->MoveSession(id, dst);
}

void Group::execute(IOCP* iocp)
{
	_executable.Execute(0, executable::ExecutableTransfer::GROUP, iocp);
}


