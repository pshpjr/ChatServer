#include "stdafx.h"
#include "PostSendExecutable.h"
#include "Session.h"
#include "CSendBuffer.h"
#include "IOCP.h"

void PostSendExecutable::Execute(ULONG_PTR key, DWORD transferred, void* iocp)
{
	IOCP& server = *reinterpret_cast< IOCP* >( iocp );
	Session* session = ( Session* ) key;
	auto& sendingQ = session->_sendingQ;

	CSendBuffer* buffer = nullptr;

	for ( int i = 0; i < dataNotSend; i++ )
	{
		sendingQ[i]->Release(L"PostSendRelease");
	}
	server.increaseSendCount(dataNotSend);

	dataNotSend = 0;

	session->needCheckSendTimeout = false;

	int oldSending = InterlockedExchange(&session->_isSending, 0);
	if ( oldSending != 1 )
	{
		DebugBreak();
	}

	session->trySend();
	session->Release(L"PostSendRelease");
}