#include "stdafx.h"
#include "SendExecutable.h"
#include "Session.h"

void SendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{

	Session* session = ( Session* ) key;


	session->trySend();
}