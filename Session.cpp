#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSerializeBuffer.h"
#include "IOCP.h"
#include "Protocol.h"
#include <cstdio>
Session::Session() : _owner(nullptr), _socket({}), _sessionID(-1), _recvBuffer(nullptr)
{

}

Session::Session(Socket socket, uint64 sessionId, IOCP& owner) : _socket(socket), _sessionID(sessionId), _owner(&owner),_recvBuffer(nullptr)
{ 

}

void Session::Enqueue(CSerializeBuffer* buffer)
{
	_sendQ.Enqueue(buffer);
}

void Session::Close()
{
	InterlockedExchange8(&_connect,0);
	DebugBreak();
	_socket.CancleIO();
}

bool Session::Release(LPCWSTR content, int type)
{
	int refDecResult = InterlockedDecrement(&_refCount);

#ifdef SESSION_DEBUG

	auto index = InterlockedIncrement(&debugIndex);
	release_D[index % debugSize] = { refDecResult,content,type };
#endif

	if (refDecResult == 0)
	{
		if (InterlockedCompareExchange(&_refCount, releaseFlag, 0) == 0)
		{

			PostQueuedCompletionStatus(_owner->_iocp, -1, (ULONG_PTR)this, &_releaseExecutable._overlapped);

			return true;
		}
	}
	return false;
}


const int MAX_SEND_COUNT = 50;



void Session::trySend()
{
	if (_connect == false) 
	{
		return;
	}
	while (true)
	{
		//이걸로 disconnect시 전송 을 방지할 수 있을까?
		if(_refCount & releaseFlag)
		{
			return;
		}

		if (_sendQ.Size() == 0)
		{

			return;
		}
		if (InterlockedExchange(&_isSending, true) == 1)
		{

			return;
		}

		if (_sendQ.Size() == 0)
		{

			InterlockedExchange(&_isSending, false);

			continue;
		}
		break;
	}
	
	auto refIncResult = IncreaseRef(L"realSendInc");
	if (refIncResult >= releaseFlag) 
	{
		Release(L"realSendSessionisRelease");
		return;
	}

	int sendPackets = 0;
	WSABUF sendWsaBuf[MAX_SEND_COUNT] = {};
	for (int i = 0; i < MAX_SEND_COUNT; i++)
	{
		CSerializeBuffer* buffer;
		if (!_sendQ.Dequeue(buffer))
		{
			break;
		}
		sendPackets++;
		_sendingQ.Enqueue(buffer);

		if (_staticKey) 
		{
			buffer->Encode(_staticKey);
		}
		else 
		{
			buffer->writeLanHeader();
		}
		

		sendWsaBuf[i].buf = buffer->GetHead();
		sendWsaBuf[i].len = buffer->SendDataSize();

		ASSERT_CRASH(sendWsaBuf[i].len > 0, "Out of Case");
	}



	if (sendPackets == 0)
	{
		InterlockedExchange(&_isSending, false);
		Release(L"realSendPacket0Release");
		return;
	}


	needCheckSendTimeout = true;
	_postSendExecute.lastSend = chrono::system_clock::now();

	DWORD flags = 0;
	_postSendExecute.Clear();

	int sendResult = _socket.Send(sendWsaBuf, sendPackets, flags ,&_postSendExecute._overlapped);
	if (sendResult == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSA_IO_PENDING) 
		{
			return;
		}

		switch (error) {
		case WSAECONNABORTED:
			__fallthrough;
		case WSAECONNRESET:
			__fallthrough;
			break;
		default:
			DebugBreak();
		}
		Close();
		Release(L"SendErrorRelease");
	}
}

void Session::registerRecv()
{
	InterlockedIncrement(&_refCount);

	RecvNotIncrease();
}

void Session::RecvNotIncrease()
{
	if (_connect == false) {
		Release(L"tryRecvReleaseIOCancled");
		return;
	}

	_recvExecute.Clear();

	WSABUF recvWsaBuf {};
	DWORD flags = 0;



	recvWsaBuf.buf = _recvBuffer->GetRear();
	recvWsaBuf.len = _recvBuffer->canPushSize();

	lastRecv = chrono::system_clock::now();

	int recvResult = _socket.Recv(&recvWsaBuf, 1, &flags, &_recvExecute._overlapped);
	if (recvResult == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSA_IO_PENDING)
		{
			return;
		}

		switch (error) {
		case WSAECONNABORTED:
			__fallthrough;

		case WSAECONNRESET:
			__fallthrough;
			break;
		default:
		{
			DebugBreak();
			int i = 0;
		}
		}
		Release(L"RecvErrorRelease");
	}
}


void Session::RegisterIOCP(HANDLE iocpHandle)
{
	CreateIoCompletionPort((HANDLE)_socket._socket, iocpHandle, (ULONG_PTR)this, 0);
}

void Session::Reset()
{
	//debugIndex = 0;
	_sendExecute.Clear();
	_recvExecute.Clear();
	_postSendExecute.Clear();
	_recvBuffer->Release(L"SessionResetRelease");
	_isSending = false;
	needCheckSendTimeout = false;
	_timeout = _defaultTimeout;



	CSerializeBuffer* sendBuffer;
	while (_sendQ.Dequeue(sendBuffer))
	{
		sendBuffer->Release(L"ResetSendRelease");
	}
	_recvQ.Clear();
	while (_sendingQ.Dequeue(sendBuffer))
	{
		sendBuffer->Release(L"ResetSendingRelease");
	}
	_socket.Close();
}

