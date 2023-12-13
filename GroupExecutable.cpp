#include "stdafx.h"
#include "GroupExecutable.h"
#include "IOCP.h"
#include "Group.h"

void GroupExecutable::Execute(ULONG_PTR arg, DWORD transferred, void* iocp)
{
	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
	_owner->Update();

	server.PostExecutable(this, executable::ExecutableTransfer::GROUP);

}
