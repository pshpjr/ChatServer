#pragma once
#include <memory>
#include "BuildOption.h"
#include "Types.h"
#include "Group.h"

class GroupManager
{
	friend class Group;
public:
	GroupManager(IOCP* owner);


	template <typename GroupType, typename ...Args>
	GroupID CreateGroup(Args&&... args);

	//move 0이면 나가기. 
	void MoveSession(SessionID target, GroupID dst,bool update = true);
	void SessionDisconnected(Session* target);

	void Update();

	bool OnRecv(SessionID sessionId, GroupID groupId);
private:


	SRWLOCK _groupLock;
	HashMap<GroupID, unique_ptr<Group>> _groups;

	IOCP* _owner;
	
	/// <summary>
	/// 0번 그룹은 그룹이 없는 것을 의미한다. 
	/// </summary>
	GroupID g_groupID = InvalidGroupID();
};


template <typename GroupType, typename ...Args>
GroupID GroupManager::CreateGroup(Args&&... args)
{
	static_assert( is_base_of_v<Group, GroupType>, "GroupType must inherit Group" );

	Group* newGroup = new GroupType(std::forward<Args>(args)...);
	newGroup->_groupId = NextID();
	newGroup->_iocp = _owner;
	newGroup->_owner = this;

	AcquireSRWLockExclusive(&_groupLock);
	auto ret = _groups.emplace(newGroup->_groupId, unique_ptr<Group>(newGroup));
	ReleaseSRWLockExclusive(&_groupLock);

	newGroup->OnCreate();
	return newGroup->_groupId;
}
