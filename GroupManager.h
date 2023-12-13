#pragma once
#include <memory>
#include "BuildOption.h"
#include "Types.h"
#include "Group.h"

class GroupManager
{
public:
	GroupManager(IOCP* owner);


	template <typename GroupType>
	short CreateGroup();
	template <typename ClientType>
	ClientType& CreateClient(String ip, Port port);
	//move 0이면 나가기. 
	void MoveSession(SessionID target, GroupID dst);

private:
	SessionID createSession(String ip, Port port);

	SRWLOCK _groupLock;
	HashMap<GroupID, unique_ptr<Group>> _groups;

	IOCP* _owner;
	
	short g_groupID = 2;
};

template <typename ClientType>
ClientType& GroupManager::CreateClient(String ip, Port port)
{
	auto newGroup = new ClientType();
	newGroup->_groupId = InterlockedIncrement16(&g_groupID);
	newGroup->_iocp = _owner;
	newGroup->_owner = this;
	newGroup->_id = createSession(ip, port);


	AcquireSRWLockExclusive(&_groupLock);
	auto ret = _groups.emplace(newGroup->_groupId, unique_ptr<Group>(newGroup));
	ReleaseSRWLockExclusive(&_groupLock);

	newGroup->execute(_owner);
	return *newGroup;
}