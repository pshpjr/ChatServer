#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSendBuffer.h"
#include "IOCP.h"
#include <optional>
#include "Executables.h"
#include "CRecvBuffer.h"
Session::Session() : _sessionId(), _socket({}), _recvTempBuffer(), _sendingQ(), _owner(nullptr)
,_recvExecute(*new RecvExecutable),_postSendExecute(*new PostSendExecutable),_sendExecute(*new SendExecutable), _releaseExecutable(*new ReleaseExecutable)
{

}

Session::Session(Socket socket, SessionID sessionId, IOCP& owner) : _sessionId(sessionId), _socket(socket), _recvTempBuffer(), _sendingQ(), _owner(&owner)
, _recvExecute(*new RecvExecutable), _postSendExecute(*new PostSendExecutable), _sendExecute(*new SendExecutable), _releaseExecutable(*new ReleaseExecutable)
{

}

bool Session::CanSend()
{
	if ( _connect == false )
	{
		return false;
	}
	while ( true )
	{
		//이걸로 disconnect시 전송 을 방지할 수 있을까?
		if ( _refCount >= RELEASE_FLAG )
		{
			return false;
		}

		if ( _sendQ.Size() == 0 )
		{
			return false;
		}
		if ( InterlockedExchange8(&_isSending, true) == 1 )
		{
			return false;
		}

		if ( _sendQ.Size() == 0 )
		{

			InterlockedExchange8(&_isSending, false);

			continue;
		}
		break;
	}
	return true;
}

void Session::EnqueueSendData(CSendBuffer* buffer)
{
	_sendQ.Enqueue(buffer);
}

void Session::Close()
{
	InterlockedExchange8(&_connect, 0);
	_socket.CancelIO();
}




void Session::TrySend()
{
	if ( !CanSend() )
	{
		EASY_BLOCK("CANSEND")
		return;
	}

	RealSend();
}

void Session::RegisterRecv()
{
	InterlockedIncrement(&_refCount);

	RecvNotIncrease();
}

void Session::RecvNotIncrease()
{
	if ( _connect == false )
	{
		Release(L"tryRecvReleaseIOCanceled");
		return;
	}
	EASY_BLOCK("RECV")
	int bufferCount = 1;
	WSABUF recvWsaBuf[2] = {};
	DWORD flags = 0;

	recvWsaBuf[0].buf = _recvQ.GetRear();
	recvWsaBuf[0].len = _recvQ.DirectEnqueueSize();

	if ( _recvQ.FreeSize() > recvWsaBuf[0].len )
	{
		recvWsaBuf[1].buf = _recvQ.GetBuffer();
		recvWsaBuf[1].len = _recvQ.FreeSize() - recvWsaBuf[0].len;

		bufferCount = 2;
	}
	_recvExecute.Clear();


	lastRecv = chrono::system_clock::now();

	if (const int recvResult = _socket.Recv(recvWsaBuf, bufferCount, &flags, &_recvExecute._overlapped); recvResult == SOCKET_ERROR )
	{
		const int error = WSAGetLastError();
		if ( error == WSA_IO_PENDING )
		{
			return;
		}

		switch ( error )
		{
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


void Session::RegisterIOCP(const HANDLE iocpHandle)
{
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket._socket), iocpHandle, ( ULONG_PTR ) this, 0);
}

void Session::SetDefaultTimeout(const int timeoutMillisecond)
{
	_defaultTimeout = timeoutMillisecond;
}

void Session::SetTimeout(const int timeoutMillisecond)
{
	_timeout = timeoutMillisecond;
}

void Session::ResetTimeoutWait()
{
	const auto now = chrono::system_clock::now();
	lastRecv = now;
	_postSendExecute.lastSend = now;
}

bool Session::CheckTimeout(const chrono::system_clock::time_point now)
{
	if ( _timeout == 0 )
	{
		return false;
	}
	if ( _refCount >= RELEASE_FLAG )
	{
		return false;
	}

	if ( IncreaseRef(L"timeoutInc") >= RELEASE_FLAG )
	{
		Release(L"TimeoutAlreadyRelease");
		return false;
	}

	const auto recvWait = chrono::duration_cast< chrono::milliseconds >( now - lastRecv );
	const auto sendWait = chrono::duration_cast< chrono::milliseconds >( now - _postSendExecute.lastSend );

	bool isTimedOut = false;

	if ( needCheckSendTimeout && sendWait.count() > _timeout )
	{
		isTimedOut = true;
	}

	if ( recvWait.count() > _timeout )
	{
		isTimedOut = true;
	}

	Release(L"TimeoutRelease");
	return isTimedOut;
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

	
	//GroupID _groupID = 0;

	CRecvBuffer* recvBuffer;
	while ( _groupRecvQ.Dequeue(recvBuffer) )
	{
		recvBuffer->Release(L"ResetRecvRelease");
	}

	CSendBuffer* sendBuffer;
	while ( _sendQ.Dequeue(sendBuffer) )
	{
		sendBuffer->Release(L"ResetSendRelease");
	}
	_recvQ.Clear();

	const int notSend = InterlockedExchange(&_postSendExecute.dataNotSend, 0);

	for ( int i = 0; i < notSend; i++ )
	{
		_sendingQ[i]->Release(L"ResetSendingRelease");
		_sendingQ[i] = nullptr;
	}

	_socket.Close();
}

bool Session::Release([[maybe_unused]] LPCWSTR content, [[maybe_unused]] int type)
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
			InterlockedIncrement(&_owner->_iocpCompBufferSize);
			PostQueuedCompletionStatus(_owner->_iocp, -1, reinterpret_cast<ULONG_PTR>(this), &_releaseExecutable._overlapped);

			return true;
		}
	}
	return false;
}

void Session::RealSend()
{
	EASY_FUNCTION();
	if (const auto refIncResult = IncreaseRef(L"realSendInc"); refIncResult >= RELEASE_FLAG )
	{
		Release(L"realSendSessionIsRelease");
		return;
	}

	int sendPackets = 0;
	WSABUF sendWsaBuf[MAX_SEND_COUNT] = {};
	for ( int i = 0; i < MAX_SEND_COUNT; i++ )
	{
		CSendBuffer* buffer;
		if ( !_sendQ.Dequeue(buffer) )
		{
			break;
		}
		sendPackets++;

		_sendingQ[i] = buffer;



		if ( _staticKey )
		{
			buffer->TryEncode(_staticKey);
		}
		else
		{
			buffer->WriteLanHeader();
		}

		sendWsaBuf[i].buf = buffer->GetHead();
		sendWsaBuf[i].len = buffer->SendDataSize();

		ASSERT_CRASH(sendWsaBuf[i].len > 0, "Out of Case");
	}

	if ( sendPackets == 0 )
	{ 
		InterlockedExchange8(&_isSending, false);
		Release(L"realSendPacket0Release");
		return;
	}

	needCheckSendTimeout = true;
	_postSendExecute.lastSend = chrono::system_clock::now();

	constexpr DWORD flags = 0;
	_postSendExecute.Clear();

	_postSendExecute.dataNotSend = sendPackets;

	EASY_BLOCK("WSASEND");
	if (const int sendResult = _socket.Send(sendWsaBuf, sendPackets, flags, &_postSendExecute._overlapped); sendResult == SOCKET_ERROR )
	{
		const int error = WSAGetLastError();
		if ( error == WSA_IO_PENDING )
		{
			return;
		}

		switch ( error )
		{
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