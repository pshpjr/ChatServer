#include "stdafx.h"
#include "GroupExecutable.h"
#include "IOCP.h"
#include "Group.h"

void GroupExecutable::Execute(ULONG_PTR arg, DWORD transferred, void* iocp)
{
	using std::chrono::steady_clock;
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;

	auto start = steady_clock::now();

	_owner->Update();

	auto end = steady_clock::now();

	auto waitTime = EXECUTION_INTERVAL - duration_cast<milliseconds>(end - start);

	if ( waitTime.count() > 0 )
	{
		Sleep(waitTime.count());
	}

	const IOCP& server = *static_cast< IOCP* >( iocp );
	server.PostExecutable(this, executable::ExecutableTransfer::GROUP);
}
