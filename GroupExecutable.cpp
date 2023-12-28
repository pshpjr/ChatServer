#include "stdafx.h"
#include "GroupExecutable.h"
#include "IOCP.h"
#include "Group.h"

void GroupExecutable::Execute(ULONG_PTR arg, DWORD transferred, void* iocp)
{
	const IOCP& server = *static_cast< IOCP* >( iocp );
	_owner->Update();

	server.PostExecutable(this, executable::ExecutableTransfer::GROUP);

}
