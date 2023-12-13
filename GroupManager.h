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

