#include "stdafx.h"
#include "GroupManager.h"
#include "GroupExecutable.h"
#include "IOCP.h"



void GroupManager::MoveSession(const SessionID target, const GroupID dst, bool update)
{
	const auto session = _owner->FindSession(target, L"MoveSession");

	if ( session == nullptr )
	{
		return;
	}
	GroupID src = session->GetGroupID();

	
	AcquireSRWLockShared(&_groupLock);

	if (src != 0)
	{

		const auto& srcGroup = _groups.find(src)->second;
#ifdef SESSION_DEBUG
		session->Write(-1, src, L"MoveLeave");
#endif
		srcGroup->LeaveSession(session->GetSessionId(), update);
	}
	session->SetGroupID(dst);
	if(dst != 0)
	{
		const auto& dstGroup = _groups.find(dst)->second;
#ifdef SESSION_DEBUG
		session->Write(-1, dst, L"MoveEnter");
#endif
		dstGroup->EnterSession(session->GetSessionId(),update);
	}
	ReleaseSRWLockShared(&_groupLock);



	session->Release(L"MoveSessionRel");
}


void GroupManager::Update()
{
	auto now = ::chrono::steady_clock::now();
	AcquireSRWLockShared(&_groupLock);
	for (auto& [groupId, groupPtr] : _groups)
	{
		if (groupPtr->NeedUpdate()) 
		{
			groupPtr->_executable.Clear();
			_owner->PostExecutable((Executable*)(&(groupPtr->_executable)), 0);
		}
	}
	ReleaseSRWLockShared(&_groupLock);
}

bool GroupManager::OnRecv(SessionID sessionId, GroupID groupId)
{
	return _groups[groupId]->Enqueue({ Group::GroupJob::Recv,sessionId });
}

void GroupManager::SessionDisconnected(Session* target)
{
	GroupID src = target->GetGroupID();

	AcquireSRWLockShared(&_groupLock);

	if (src != 0)
	{
		auto& srcGroup = _groups.find(src)->second;
#ifdef SESSION_DEBUG
		target->Write(-1, src, L"Disconnect");
#endif
		srcGroup->LeaveSession(target->GetSessionId(), true);
	}

	ReleaseSRWLockShared(&_groupLock);
}


GroupManager::GroupManager(IOCP* owner) : _owner(owner)
{
	InitializeSRWLock(&_groupLock);
}


