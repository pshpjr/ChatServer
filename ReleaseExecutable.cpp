#include "stdafx.h"
#include "ReleaseExecutable.h"
#include "Session.h"
#include "IOCP.h"

void ReleaseExecutable::Execute(const ULONG_PTR key, DWORD transferred, void* iocp)
{
	EASY_FUNCTION();
	const auto session = reinterpret_cast<Session*>(key);
	//InterlockedDecrement(&session->_owner->_iocpCompBufferSize);

	session->_owner->_onDisconnect(session->_sessionId);

	session->Reset();
	const unsigned short index = session->_sessionId.index;
	session->_owner->freeIndex.Push(index);
}