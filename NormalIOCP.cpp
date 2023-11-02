#include "stdafx.h"
#include "NormalIOCP.h"

#include <chrono>
#include <process.h>

#include "IOCP.h"
#include "Session.h"
#include "CSerializeBuffer.h"
#include "LockGuard.h"

bool IOCP::Init(String ip, Port port, uint16 backlog, uint16 maxNetThread, char staticKey)
{
	_isRunning = true;
	_maxNetThread = maxNetThread;
	_staticKey = staticKey;
	
	
	if (staticKey) {
		for (auto& session : sessions) {
			session.SetNetSession(staticKey);
		}
	}

	_threadArray = new HANDLE[_maxNetThread+2];


	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
	{
		return false;
	}


	_listenSocket.Init();
	if (_listenSocket.isValid() == false)
	{
		return false;
	}

	if (_listenSocket.Bind(ip, port) == false)
	{
		_listenSocket.CancleIO();
		return false;
	}

	if (_listenSocket.Listen(backlog)== false)
	{
		_listenSocket.CancleIO();
		return false;
	}

	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (_iocp == INVALID_HANDLE_VALUE)
	{
		_listenSocket.CancleIO();
		return false;
	}

	void* _this = this;

	_threadArray[0] = (HANDLE)_beginthreadex(nullptr,0,AcceptEntry,_this, 0, nullptr);
	if (_threadArray[0] == INVALID_HANDLE_VALUE)
	{
		_listenSocket.CancleIO();
		return false;
	}
	printf("AcceptThread %p\n", _threadArray[0]);
	_threadArray[1] = (HANDLE)_beginthreadex(nullptr, 0, MonitorEntry, _this, 0, nullptr);
	if (_threadArray[1] == INVALID_HANDLE_VALUE)
	{
		_listenSocket.CancleIO();
		return false;
	}
	printf("MonitorThread %p\n", _threadArray[1]);
	for (int i = 2; i < maxNetThread+2; ++i)
	{
		_threadArray[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerEntry, _this, 0, nullptr);
		if (_threadArray[i] == INVALID_HANDLE_VALUE)
		{
			_listenSocket.CancleIO();
			return false;
		}
		printf("WorkerThread %p\n", _threadArray[i]);
	}
	OnInit();

	return true;
}

void IOCP::Start()
{
	_isRunning = true;
	OnStart();
}

void IOCP::Stop()
{
	_isRunning = false;
	_listenSocket.CancleIO();
	PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
	auto result = WaitForMultipleObjects(_maxNetThread, _threadArray, true, INFINITE);
	if( result==WAIT_FAILED)
	{
		printf("WaitForMultipleObjects failed %d\n", GetLastError());
	}
	OnEnd();
}

int Sendflag = 0;
bool IOCP::SendPacket(SessionID sessionId, CSerializeBuffer* buffer)
{
	auto sessionIndex = (sessionId >> 47);
	auto& session = sessions[sessionIndex];
	auto refResult = session.IncreaseRef();
	auto sessionID = session.GetSessionID();

	if (session.GetSessionID() != sessionId)
	{
		session.Release();
		buffer->Release();
		return false;
	}
	buffer->IncreaseRef();
	

	if (refResult > releaseFlag)
	{
		//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
		session.Release();
		buffer->Release();
		return false;
	}

	int size = buffer->GetDataSize();
	if (size == 0) 
	{
		DebugBreak();
	}

	session.Enqueue(buffer);
	session.dataNotSend++;

	PostQueuedCompletionStatus(_iocp, -1, (ULONG_PTR)&session, &session._sendExecute._overlapped);
	return true;
}

bool IOCP::DisconnectSession(SessionID sessionId)
{
	auto sessionIndex = (sessionId >> 47);

	sessions[sessionIndex].Close();

	return true;  
}


int64 IOCP::GetAcceptTps()
{
	return _acceptTps;
}

int64 IOCP::GetRecvTps()
{
	return _recvTps;
}

int64 IOCP::GetSendTps()
{
	return _sendTps;
}

void IOCP::onDisconnect(SessionID sessionId)
{
	InterlockedDecrement16(&_sessionCount);
	OnDisconnect(sessionId); 
}

void IOCP::WorkerThread(LPVOID arg)
{
	srand(GetCurrentThreadId());

	while (true)
	{
		DWORD transferred = 0;
		Executable* overlapped = nullptr;
		Session* session = nullptr;

		auto ret = GetQueuedCompletionStatus(_iocp, &transferred, (PULONG_PTR) &session, (LPOVERLAPPED*)&overlapped, INFINITE);

		if(transferred==0&&session== nullptr&&overlapped==nullptr)
		{
			PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
			break;
		}

		if(transferred == 0 && session != nullptr)
		{
			auto sessionID =session->GetSessionID();
			if (session->Release())
			{

				onDisconnect(sessionID);
			}
		}
		else
		{
			overlapped = (Executable*)((char*)overlapped - sizeof(LPVOID));
			overlapped->Execute((PULONG_PTR)session, transferred, this);
		}

	}
	printf("WorkerThread End\n");
}

//iocp 에서 완료 통지가 올 때 처리는 소켓마다 달라진다 

void IOCP::AcceptThread(LPVOID arg)
{
	while (_isRunning)
	{
		Socket clientSocket = _listenSocket.Accept();

		if (OnAccept(clientSocket.GetSockAddr()) == false)
		{
			//TODO: 여기서 cancleIO가 의미 없음. 
			clientSocket.CancleIO();
			continue;
		}
		_acceptCount++;

		short sessionIndex;
		if (freeIndex.Pop(sessionIndex) == false) 
		{
			printf("!!!!!serverIsFull!!!!!\n");
			clientSocket.CancleIO();
			continue;
		}

		uint64 sessionID = (uint64)sessionIndex << 47;
		sessionID |= (g_sessionId++);


		sessions[sessionIndex].SetOwner(*(IOCP*)(this));
		sessions[sessionIndex].SetSessionID(sessionID);
		sessions[sessionIndex].SetSocket(clientSocket);
		sessions[sessionIndex].RegisterIOCP(_iocp);
		auto refResult = sessions[sessionIndex].IncreaseRef();


	

		sessions[sessionIndex].OffReleaseFlag();
		OnConnect(sessions[sessionIndex].GetSessionID());
		sessions[sessionIndex].RecvNotIncrease();

		InterlockedIncrement16(&_sessionCount);
	}
	printf("AcceptThreadEnd\n");
}

void IOCP::MonitorThread(LPVOID arg)
{
	while (_isRunning)
	{
		_acceptTps = _acceptCount;
		_recvTps = _recvCount;
		_sendTps = _sendCount;

		InterlockedExchange(&_acceptCount, 0);
		InterlockedExchange(&_recvCount, 0);
		InterlockedExchange(&_sendCount, 0);


		Sleep(1000);
	}
	printf("MonitorThread End\n");

}

unsigned __stdcall IOCP::MonitorEntry(LPVOID arg)
{
	IOCP* iocp = (IOCP*)arg;
	iocp->MonitorThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::WorkerEntry(LPVOID arg)
{
	IOCP* iocp = (IOCP*)arg;
	iocp->WorkerThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::AcceptEntry(LPVOID arg)
{
	IOCP* iocp = (IOCP*)arg;
	iocp->AcceptThread(nullptr);
	return 0;
}

IOCP::IOCP() 
{
	for (int i = MAX_SESSIONS-1; i >=0 ; i--)
	{
		freeIndex.Push(i);
	}
}
 
NormalIOCP::~NormalIOCP()
{

	CloseHandle(_iocp);
	for (int i = 0; i < _maxNetThread+2; ++i)
	{
		printf("CloseHandle %p\n", _threadArray[i]);

		CloseHandle(_threadArray[i]);
	}

	delete[] _threadArray;
}


Session* NormalIOCP::FindSession(uint64 id)
{
	auto sessionIndex = id & idMask;

	return &sessions[sessionIndex];
}



