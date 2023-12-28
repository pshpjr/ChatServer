#include "stdafx.h"
#include "PostSendExecutable.h"
#include "Session.h"
#include "CSendBuffer.h"
#include "IOCP.h"

void PostSendExecutable::Execute(const ULONG_PTR key, DWORD transferred, void* iocp)
{
	EASY_FUNCTION();

	IOCP& server = *static_cast< IOCP* >( iocp );
	const auto session = reinterpret_cast<Session*>(key);
	const auto& sendingQ = session->_sendingQ;

	CSendBuffer* buffer = nullptr;


	for ( int i = 0; i < dataNotSend; i++ )
	{
		sendingQ[i]->Release(L"PostSendRelease");
	}
	server.increaseSendCount(dataNotSend);

	dataNotSend = 0;

	session->needCheckSendTimeout = false;

	session->_isSending = 0;

	session->TrySend();

	session->Release(L"PostSendRelease");
}