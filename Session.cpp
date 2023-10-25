#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSerializeBuffer.h"
#include "IOCP.h"


Session::Session(Socket socket, uint64 sessionId, IOCP& owner) : _socket(socket), _sessionID(sessionId), _owner(owner)
{
	InitializeCriticalSection(&_lock);
}

void Session::Enqueue(CSerializeBuffer* buffer)
{
	Lock();
	_sendQ.Enqueue(buffer);
	Unlock();
}

void Session::Close()
{
	Lock();
	Unlock();

	_socket.Close();
}

bool Session::Release()
{
	int refDecResult = InterlockedDecrement(&_refCount);

	//refCount만으로 이렇게 처리해도 되는가?
	if (refDecResult == 0)
	{
		_owner.DeleteSession(_sessionID);

		Close();
		delete this;

		return true;
	}
	return false;
}


const int MAX_SEND_COUNT = 50;



void Session::trySend()
{

	while (true)
	{
		//이걸로 disconnect시 전송 을 방지할 수 있을까?
		if(_refCount == 0)
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

	//이 지점에 둘이 들어올 수 있나?
	int sendPackets = 0;
	WSABUF sendWsaBuf[MAX_SEND_COUNT];
	for (int i = 0; i < MAX_SEND_COUNT; i++)
	{
		CSerializeBuffer* buffer;
		if (!_sendQ.Dequeue(buffer))
		{
			break;
		}
		sendPackets++;
		_sendingQ.Enqueue(buffer);

		sendWsaBuf[i].buf = buffer->GetFullBuffer();
		sendWsaBuf[i].len = buffer->GetFullSize();
		ASSERT_CRASH(sendWsaBuf[i].len > 0, "Out of Case");
	}

	if (sendPackets == 0)
	{
		InterlockedExchange(&_isSending, false);
		return;
	}



	InterlockedIncrement(&_refCount);
	_postSendExecute.isSend = true;

	for (int i = 0; i < sendPackets; ++i)
	{
		if((*sendWsaBuf[i].buf) == 0xdd)
		{
			DebugBreak();
		}
	}

	DWORD flags = 0;
	_postSendExecute.Clear();
	ASSERT_CRASH(_postSendExecute._overlapped.InternalHigh == 0);

	int sendResult = _socket.Send(sendWsaBuf, sendPackets, flags ,&_postSendExecute._overlapped);
	if (sendResult == SOCKET_ERROR)
	{
		printf("SocketError\n");
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			printf("Send_ERROR\n");
			return;
		}

		Release();
	}

}

void Session::registerRecv()
{
	InterlockedIncrement(&_refCount);

	_postRecvNotIncrease();
}

void Session::_postRecvNotIncrease()
{
	_recvExecute.Clear();
	WSABUF recvWsaBuf[2];
	DWORD flags = 0;
	char bufferCount = 1;


	recvWsaBuf[0].buf = _recvQ.GetRear(); //recv 1회시 불변
	recvWsaBuf[0].len = _recvQ.DirectEnqueueSize();// 스레드 상황에 따라 변경 가능. 늘어나기만 함. 

	//넣을 수 있는 공간이 잘린 상황이 front가 rear보다 항상 앞에 있다고 생각해도 되는가?
	//그런 듯 함
	//getFront 함수를 써도 문제 없는 이유는 지금 한 세션에 한 스레드만 접근한다고 가정하고 있기 때문. 
	if (_recvQ.FreeSize() > recvWsaBuf[0].len)
	{
		recvWsaBuf[1].buf = _recvQ.GetBuffer();
		recvWsaBuf[1].len = _recvQ.FreeSize() - recvWsaBuf[0].len;

		bufferCount = 2;
	}

	int recvResult = _socket.Recv(recvWsaBuf, bufferCount, &flags, &_recvExecute._overlapped);
	if (recvResult == SOCKET_ERROR)
	{

		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			Release();
		}
	}
}


void Session::RegisterIOCP(HANDLE iocpHandle)
{
	CreateIoCompletionPort((HANDLE)_socket._socket, iocpHandle, (ULONG_PTR)this, 0);
}

