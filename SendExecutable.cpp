#include "stdafx.h"
#include "SendExecutable.h"
#include "Session.h"
#include "IOCP.h"
void SendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	EASY_FUNCTION();

	Session* session = ( Session* ) key;
	//InterlockedDecrement(&session->_owner->_iocpCompBufferSize);
	session->RealSend();
}