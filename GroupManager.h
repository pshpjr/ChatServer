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

	//move 0이면 나가기. 
	void MoveSession(SessionID target, GroupID dst);

private:
	SRWLOCK _groupLock;
	HashMap<GroupID, unique_ptr<Group>> _groups;

	IOCP* _owner;
	
	short g_groupID = 2;
};

template <typename GroupType>
short GroupManager::CreateGroup()
{
	static_assert( is_base_of_v<Group, GroupType>, "GroupName must inherit Group" );

	Group* newGroup = new GroupType();
	newGroup->_groupId = InterlockedIncrement16(&g_groupID);
	newGroup->_iocp = _owner;
	newGroup->_owner = this;

	AcquireSRWLockExclusive(&_groupLock);
	auto ret = _groups.emplace(newGroup->_groupId, unique_ptr<Group>(newGroup));
	ReleaseSRWLockExclusive(&_groupLock);

	newGroup->execute(_owner);
	return newGroup->_groupId;
}
