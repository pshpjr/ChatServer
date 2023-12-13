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

SessionID GroupManager::createSession(String ip, Port port)
{
	return _owner->createClientSession(ip, port);
}

GroupManager::GroupManager(IOCP* owner) : _owner(owner)
{
	InitializeSRWLock(&_groupLock);
}

template <typename GroupType>
short GroupManager::CreateGroup()
{
	static_assert( is_base_of_v<Group, GroupType>, "GroupName must inherit Group" );

	auto newGroup = new GroupType();
	newGroup->_groupId = InterlockedIncrement16(g_groupID);
	newGroup->_iocp = _owner;
	newGroup->_owner = this;

	AcquireSRWLockExclusive(&_groupLock);
	auto ret = _groups.emplace(newGroup->_groupId, unique_ptr<GroupType>(newGroup));
	ReleaseSRWLockExclusive(&_groupLock);

	newGroup->execute(_owner);
	return newGroup->_groupID;
}

