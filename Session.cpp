#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSendBuffer.h"
#include "IOCP.h"
#include "Protocol.h"


Session::Session() : _owner(nullptr), _socket({}), _sessionID(-1)
{

}

Session::Session(Socket socket, uint64 sessionId, IOCP& owner) : _socket(socket), _sessionID(sessionId), _owner(&owner)
{ 

}

void Session::EnqueueSendData(CSendBuffer* buffer)
{
	_sendQ.Enqueue(buffer);
}

void Session::Close()
{
	InterlockedExchange8(&_connect,0);
	DebugBreak();
	_socket.CancleIO();
}




void Session::trySend()
{
	if (_connect == false) 
	{
		return;
	}
	while (true)
	{
		//이걸로 disconnect시 전송 을 방지할 수 있을까?
		if(_refCount & RELEASE_FLAG)
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
	PRO_BEGIN("TrySend");
	auto refIncResult = IncreaseRef(L"realSendInc");
	if (refIncResult >= RELEASE_FLAG) 
	{
		Release(L"realSendSessionisRelease");
		return;
	}

	int sendPackets = 0;
	WSABUF sendWsaBuf[MAX_SEND_COUNT] = {};
	for (int i = 0; i < MAX_SEND_COUNT; i++)
	{
		CSendBuffer* buffer;
		if (!_sendQ.Dequeue(buffer))
		{
			break;
		}
		sendPackets++;

		_sendingQ[i] = buffer;

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

	dataNotSend = sendPackets;

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

	recvWsaBuf.buf = _recvQ.GetFront();
	recvWsaBuf.len = _recvQ.DirectEnqueueSize();

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

void Session::SetDefaultTimeout(int timoutMillisecond)
{
	_defaultTimeout = timoutMillisecond;
}

void Session::SetTimeout(int timoutMillisecond)
{
	_timeout = timoutMillisecond;
}

void Session::ResetTimeoutwait()
{
	auto now = chrono::system_clock::now();
	lastRecv = now;
	_postSendExecute.lastSend = now;
}

bool Session::CheckTimeout(chrono::system_clock::time_point now)
{
	IncreaseRef(L"timeoutInc");
	if ( _refCount >= RELEASE_FLAG )
	{
		Release(L"TimeoutRelease");
		return false;
	}

	auto recvWait = chrono::duration_cast< chrono::milliseconds >( now - lastRecv );
	auto sendWait = chrono::duration_cast< chrono::milliseconds >( now - _postSendExecute.lastSend );

	bool isTimeouted = false;

	if ( needCheckSendTimeout && sendWait.count() > _timeout )
	{
		isTimeouted = true;
	}

	if ( recvWait.count() > _timeout )
	{
		isTimeouted = true;
	}

	/*if ( isTimeouted )
		GLogger->write(L"Timeout", LogLevel::Debug, L"Timeouted %s %d",_socket.GetIP().c_str(), _socket.GetPort());*/


	Release(L"TimeoutRelease");
	return isTimeouted;
}

void Session::Reset()
{
	//debugIndex = 0;
	_sendExecute.Clear();
	_recvExecute.Clear();
	_postSendExecute.Clear();
	_recvQ.Clear();
	_isSending = false;
	needCheckSendTimeout = false;
	_timeout = _defaultTimeout;

	CSendBuffer* sendBuffer;
	while (_sendQ.Dequeue(sendBuffer))
	{
		sendBuffer->Release(L"ResetSendRelease");
	}
	_recvQ.Clear();

	for ( int i = 0; i < dataNotSend; i++ )
	{
		_sendingQ[i]->Release(L"ResetSendingRelease");
	}

	_socket.Close();
}

bool Session::Release(LPCWSTR content, int type /*= 0*/)
{
	int refDecResult = InterlockedDecrement(&_refCount);

#ifdef SESSION_DEBUG

	auto index = InterlockedIncrement(&debugIndex);
	release_D[index % debugSize] = { refDecResult,content,type };
#endif

	if ( refDecResult == 0 )
	{
		if ( InterlockedCompareExchange(&_refCount, RELEASE_FLAG, 0) == 0 )
		{

			PostQueuedCompletionStatus(_owner->_iocp, -1, ( ULONG_PTR ) this, &_releaseExecutable._overlapped);

			return true;
		}
	}
	return false;
}

