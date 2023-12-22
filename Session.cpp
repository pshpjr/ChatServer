#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSendBuffer.h"
#include "IOCP.h"
#include "Protocol.h"
#include <optional>
#include "Executables.h"
#include "CRecvBuffer.h"
Session::Session() : _owner(nullptr), _socket({}), _sessionID(), _sendingQ(), _recvTempBuffer()
,_recvExecute(*new RecvExecutable),_sendExecute(*new SendExecutable),_postSendExecute(*new PostSendExecutable), _releaseExecutable(*new ReleaseExecutable)
{

}

Session::Session(Socket socket, SessionID sessionId, IOCP& owner) : _socket(socket), _sessionID(sessionId), _owner(&owner), _sendingQ(), _recvTempBuffer()
, _recvExecute(*new RecvExecutable), _sendExecute(*new SendExecutable), _postSendExecute(*new PostSendExecutable), _releaseExecutable(*new ReleaseExecutable)
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
	_socket.CancleIO();
}




void Session::trySend()
{
	if ( !CanSend() )
	{
		EASY_BLOCK("CANSEND")
		return;
	}

	RealSend();
}

void Session::registerRecv()
{
	InterlockedIncrement(&_refCount);

	RecvNotIncrease();
}

void Session::RecvNotIncrease()
{
	if ( _connect == false )
	{
		Release(L"tryRecvReleaseIOCancled");
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

	int recvResult = _socket.Recv(recvWsaBuf, bufferCount, &flags, &_recvExecute._overlapped);
	if ( recvResult == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
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


void Session::RegisterIOCP(HANDLE iocpHandle)
{
	CreateIoCompletionPort(( HANDLE ) _socket._socket, iocpHandle, ( ULONG_PTR ) this, 0);
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
	_postSendExecute._lastSend = now;
}

bool Session::CheckTimeout(chrono::system_clock::time_point now)
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

	auto recvWait = chrono::duration_cast< chrono::milliseconds >( now - lastRecv );
	auto sendWait = chrono::duration_cast< chrono::milliseconds >( now - _postSendExecute._lastSend );

	bool isTimeouted = false;

	if ( needCheckSendTimeout && sendWait.count() > _timeout )
	{
		isTimeouted = true;
	}

	if ( recvWait.count() > _timeout )
	{
		isTimeouted = true;
	}

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

	int notSend = InterlockedExchange(&_postSendExecute.dataNotSend, 0);

	for ( int i = 0; i < notSend; i++ )
	{
		_sendingQ[i]->Release(L"ResetSendingRelease");
		_sendingQ[i] = nullptr;
	}

	_socket.Close();
}

bool Session::Release(LPCWSTR content, int type)
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
			PostQueuedCompletionStatus(_owner->_iocp, -1, ( ULONG_PTR ) this, &_releaseExecutable._overlapped);

			return true;
		}
	}
	return false;
}

void Session::RealSend()
{
	EASY_FUNCTION();
	auto refIncResult = IncreaseRef(L"realSendInc");
	if ( refIncResult >= RELEASE_FLAG )
	{
		Release(L"realSendSessionisRelease");
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

	if ( sendPackets == 0 )
	{ 
		InterlockedExchange8(&_isSending, false);
		Release(L"realSendPacket0Release");
		return;
	}

	needCheckSendTimeout = true;
	_postSendExecute._lastSend = chrono::system_clock::now();

	DWORD flags = 0;
	_postSendExecute.Clear();

	_postSendExecute.dataNotSend = sendPackets;

	EASY_BLOCK("WSASEND");
	int sendResult = _socket.Send(sendWsaBuf, sendPackets, flags, &_postSendExecute._overlapped);
	if ( sendResult == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
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