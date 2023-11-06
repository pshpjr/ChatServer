#include "stdafx.h"
#include "NormalIOCP.h"

#include <chrono>
#include <process.h>

#include "IOCP.h"
#include "Session.h"
#include "CSerializeBuffer.h"
#include "LockGuard.h"
#include "CMap.h"

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
	GMap.SetOwner(this);
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
	gracefulEnd = true;

	OnEnd();
}

int Sendflag = 0;
bool IOCP::SendPacket(SessionID sessionId, CSerializeBuffer* buffer)
{
	auto sessionIndex = (sessionId >> 47);
	auto& session = sessions[sessionIndex];
	auto refResult = session.IncreaseRef(L"SendPacketInc");
	auto sessionID = session.GetSessionID();

	if (sessionID != sessionId)
	{
		session.Release(L"SendPacketSessionChange");
		return false;
	}
	if (refResult > releaseFlag)
	{
		//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
		session.Release(L"SendPacketSessionRelease");
		return false;
	}



	buffer->IncreaseRef();

	int size = buffer->GetDataSize();
	if (size == 0) 
	{
		DebugBreak();
	}


	session.Enqueue(buffer);
	session.dataNotSend++;

	PostQueuedCompletionStatus(_iocp, -1, (ULONG_PTR)&session, &session._sendExecute._overlapped);
	session.Release(L"SendPacketRelease");
	return true;
}

bool IOCP::DisconnectSession(SessionID sessionId)
{
	auto sessionIndex = (sessionId >> 47);

	sessions[sessionIndex].Close();

	return true;  
}

bool IOCP::isEnd()
{
	return gracefulEnd;
}


uint64 IOCP::GetAcceptCount()
{
	return _acceptCount;
}

uint64 IOCP::GetAcceptTps()
{
	return _acceptTps;
}

uint64 IOCP::GetRecvTps()
{
	return _recvTps;
}

uint64 IOCP::GetSendTps()
{
	return _sendTps;
}

uint16 IOCP::GetSessions()
{
	return _sessions;
}

uint64 IOCP::GetPacketPoolSize()
{
	return _packetPoolSize;
}

uint32 IOCP::GetPacketPoolEmptyCount()
{
	return _packetPoolEmpty;
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
		
			session->Release(L"GQCSerrorRelease", overlapped->_type);

		}
		else
		{
			overlapped = (Executable*)((char*)overlapped - offsetof(Executable, _overlapped));
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
		auto refResult = sessions[sessionIndex].IncreaseRef(L"AcceptInc");
		sessions[sessionIndex].OffReleaseFlag();

		OnConnect(sessions[sessionIndex].GetSessionID());
		sessions[sessionIndex].RecvNotIncrease();

		InterlockedIncrement16(&_sessionCount);
	}
	printf("AcceptThreadEnd\n");
}

void IOCP::MonitorThread(LPVOID arg)
{

	auto start = chrono::system_clock::now();
	auto next = chrono::time_point_cast<chrono::milliseconds>(start) + chrono::milliseconds(1000);
	while (_isRunning)
	{
		_acceptTps = _acceptCount - _oldAccepCount;
		_oldAccepCount = _acceptCount;
		_recvTps = _recvCount;
		_sendTps = _sendCount;
		_sessions = _sessionCount;
		_packetPoolSize = CSerializeBuffer::_pool.GetGPoolSize();
		_packetPoolEmpty = CSerializeBuffer::_pool.GetGPoolEmptyCount();

		InterlockedExchange(&_recvCount, 0);
		InterlockedExchange(&_sendCount, 0);

		auto sleepTime = chrono::duration_cast<chrono::milliseconds> (next - chrono::system_clock::now());
		next += chrono::milliseconds(1000);
		if (sleepTime.count() > 0)
			Sleep(sleepTime.count());
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



