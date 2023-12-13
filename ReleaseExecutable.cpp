#include "stdafx.h"
#include "ReleaseExecutable.h"
#include "Session.h"
#include "IOCP.h"

void ReleaseExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = ( Session* ) key;

	session->_owner->_onDisconnect(session->_sessionID);

	session->Reset();
	unsigned short index = session->_sessionID.index;
	session->_owner->freeIndex.Push(index);
}