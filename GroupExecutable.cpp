#include "stdafx.h"
#include "GroupExecutable.h"
#include "IOCP.h"
#include "Group.h"

void GroupExecutable::Execute(ULONG_PTR arg, DWORD transferred, void* iocp)
{

	_owner->Update();

}
