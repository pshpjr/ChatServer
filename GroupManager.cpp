#include "stdafx.h"
#include "GroupManager.h"
#include "GroupExecutable.h"
#include "IOCP.h"



void GroupManager::MoveSession(const SessionID target, const GroupID dst)
{
	const auto session = _owner->FindSession(target, L"MoveSession");

	if ( session == nullptr )
	{
		return;
	}
	GroupID src = session->GetGroupID();
	session->SetGroupID(dst);
	
	AcquireSRWLockShared(&_groupLock);
	if(src != 0)
	{
		const auto srcGroup = _groups.find(src);
		srcGroup->second->LeaveSession(session->GetSessionId());
	}
	if(dst != 0)
	{
		const auto dstGroup = _groups.find(dst);
		dstGroup->second->EnterSession(session->GetSessionId());
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


GroupManager::GroupManager(IOCP* owner) : _owner(owner)
{
	InitializeSRWLock(&_groupLock);
}


