#include "stdafx.h"
#include "Session.h"

#include "Container.h"
#include "CSerializeBuffer.h"
#include "IOCP.h"
#include "Protocol.h"
#include <cstdio>
Session::Session(): _owner(nullptr)
{

}

Session::Session(Socket socket, uint64 sessionId, IOCP& owner) : _socket(socket), _sessionID(sessionId), _owner(&owner)
{ 

}

void Session::Enqueue(CSerializeBuffer* buffer)
{
	_sendQ.Enqueue(buffer);
}

void Session::Close()
{
	_connect = false;

	_socket.CancleIO();
}

bool Session::Release(LPCWSTR content, int type)
{
	int refDecResult = InterlockedDecrement(&_refCount);
	
	auto index = InterlockedIncrement(&debugIndex);
	release_D[index % debugSize] = { refDecResult,content,_sessionID };

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

		if (_staticKey) 
		{
			buffer->Encode(_staticKey);
		}
		else 
		{
			buffer->writeLanHeader();
		}
		

		sendWsaBuf[i].buf = buffer->GetHead();
		sendWsaBuf[i].len = buffer->GetFullSize();

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
	ASSERT_CRASH(_postSendExecute._overlapped.InternalHigh == 0);

	int sendResult = _socket.Send(sendWsaBuf, sendPackets, flags ,&_postSendExecute._overlapped);
	if (sendResult == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSA_IO_PENDING) 
		{
			return;
		}

		switch (error) {
		//WSAECONNRESET
		case 10054:
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

	lastRecv = chrono::system_clock::now();

	int recvResult = _socket.Recv(recvWsaBuf, bufferCount, &flags, &_recvExecute._overlapped);
	if (recvResult == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSA_IO_PENDING)
		{
			return;
		}

		switch (error) {
			//WSACONNRESET
			case 10054:
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
	_isSending = false;
	needCheckSendTimeout = false;
	_timeout = 5000;



	CSerializeBuffer* sendBuffer;
	while (_sendQ.Dequeue(sendBuffer))
	{
		sendBuffer->Release();
	}
	_recvQ.Clear();
	while (_sendingQ.Dequeue(sendBuffer))
	{
		sendBuffer->Release();
	}
	_socket.Close();
}

