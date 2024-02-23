#include "stdafx.h"
#include "Session.h"
#include <optional>




#include "Container.h"
#include "CSendBuffer.h"
#include "IOCP.h"

#include "Executables.h"
#include "CRecvBuffer.h"
#include "CoreGlobal.h"
#include "CLogger.h"
Session::Session() : _sessionId(), _socket({}),  _sendingQ(), _owner(nullptr)
,_recvExecute(*new RecvExecutable),_postSendExecute(*new PostSendExecutable),_sendExecute(*new SendExecutable), _releaseExecutable(*new ReleaseExecutable)
{

}

Session::Session(Socket socket, SessionID sessionId, IOCP& owner) : _sessionId(sessionId), _socket(socket),  _sendingQ(), _owner(&owner)
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
	if ( _refCount >= RELEASE_FLAG )
	{
		Release(L"tryRecvReleaseIOCanceled");
		return;
	}
	if(_connect == false)
	{
		Release(L"RecvReleaseSessionClosed");
		return;
	}
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

	_recvExecute._recvBufSize[0] = 0;
	_recvExecute._recvBufSize[1] = 0;
	_recvExecute.isPending = 0;

	for ( int i = 0; i < bufferCount; i++ )
	{
		_recvExecute._recvBufSize[i] =  recvWsaBuf[i].len;
	}
	ASSERT_CRASH(_recvExecute._recvBufSize[0] + _recvExecute._recvBufSize[1] != 0, "RecvBuffer is 0");

	lastRecv = chrono::steady_clock::now();

	//멀티스레딩 상황에서 recv의 에러 확인(ioPending)과 connect 확인 사이에 새로운 세션으로 변경될 수 있다.
	auto beforeSessionId = _sessionId;
	if (const int recvResult = _socket.Recv(recvWsaBuf, bufferCount, &flags, &_recvExecute._overlapped); recvResult == SOCKET_ERROR )
	{
		const int error = WSAGetLastError();
		if ( error == WSA_IO_PENDING )
		{
			_recvExecute.isPending = 1;
			if(_connect == false)
			{
				_owner->DisconnectSession(beforeSessionId);
			}
			
			return;
		}

		switch ( error )
		{
			case WSAECONNABORTED:
				__fallthrough;

				//원격지에서 끊었다. 
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
	const auto now = chrono::steady_clock::now();
	lastRecv = now;
	_postSendExecute.lastSend = now;
}


optional<timeoutInfo> Session::CheckTimeout(const chrono::steady_clock::time_point now)
{
	if ( _timeout == 0 )
	{
		return {};
	}
	if ( _refCount >= RELEASE_FLAG )
	{
		return {};
	}


	if ( IncreaseRef(L"timeoutInc") >= RELEASE_FLAG )
	{
		Release(L"TimeoutAlreadyRelease");
		return {};
	}

	const auto recvWait = chrono::duration_cast< chrono::milliseconds >( now - lastRecv );
	const auto sendWait = chrono::duration_cast< chrono::milliseconds >( now - _postSendExecute.lastSend );

	optional<timeoutInfo> retVal {};
	if ( _needCheckSendTimeout && sendWait.count() > _timeout )
	{
		retVal = { timeoutInfo::IOtype::send,sendWait.count(),_timeout };
	}
	else if ( recvWait.count() > _timeout )
	{
		retVal = { timeoutInfo::IOtype::recv,recvWait.count(),_timeout };
	}

	Release(L"TimeoutRelease");
	return retVal;
}


void Session::Reset()
{
	//debugIndex = 0;
	_sendExecute.Clear();
	_recvExecute.Clear();
	_postSendExecute.Clear();
	_recvQ.Clear();
	_isSending = false;
	_needCheckSendTimeout = false;
	_timeout = _defaultTimeout;

	
	_groupId = GroupID(0);

	//CRecvBuffer* recvBuffer;
	//while ( _groupRecvQ.Dequeue(recvBuffer) )
	//{
	//	recvBuffer->Release(L"ResetRecvRelease");
	//}

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

void Session::PostRelease()
{
	PostQueuedCompletionStatus(_owner->_iocp, -1, reinterpret_cast<ULONG_PTR>(this), &_releaseExecutable._overlapped);
}

void Session::RealSend()
{
	const auto refIncResult = IncreaseRef(L"realSendInc");
	if ( refIncResult >= RELEASE_FLAG )
	{
		Release(L"realSendSessionIsRelease");
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

	ASSERT_CRASH(sendPackets != 0);

	_needCheckSendTimeout = true;
	_postSendExecute.lastSend = chrono::steady_clock::now();

	constexpr DWORD flags = 0;
	_postSendExecute.Clear();

	_postSendExecute.dataNotSend = sendPackets;

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
				//원격지에서 끊었다. 
			case WSAECONNRESET:
				__fallthrough;
				break;
			default:
				DebugBreak();
		}
		gLogger->Write(L"Disconnect", CLogger::LogLevel::Debug, L"SendFail errNo : %d, sessionID : %d", error, GetSessionId());
		Release(L"SendErrorRelease");
	}
}

