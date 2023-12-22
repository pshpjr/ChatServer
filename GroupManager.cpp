#include "stdafx.h"
#include "GroupManager.h"
#include "IOCP.h"



void GroupManager::MoveSession(SessionID target, GroupID dst)
{
	auto session = _owner->FindSession(target, L"MoveSession");

	if ( session == nullptr )
	{
		return;
	}


	session->SetGroupID(dst);
	AcquireSRWLockShared(&_groupLock);
	auto dstGroup = _groups.find(dst);
	dstGroup->second->EnterSession(session->GetSessionID());
	ReleaseSRWLockShared(&_groupLock);

	session->Release(L"MoveSessionRel");
}


GroupManager::GroupManager(IOCP* owner) : _owner(owner)
{
	InitializeSRWLock(&_groupLock);
}


