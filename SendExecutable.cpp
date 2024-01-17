#include "stdafx.h"
#include "SendExecutable.h"
#include "Session.h"
#include "IOCP.h"
void SendExecutable::Execute(const ULONG_PTR key, DWORD transferred, void* iocp)
{

	const auto session = reinterpret_cast<Session*>(key);
	//InterlockedDecrement(&session->_owner->_iocpCompBufferSize);
	session->RealSend();
}