#include "stdafx.h"
#include "NormalIOCP.h"

#include <chrono>
#include <process.h>

#include "IOCP.h"
#include "Session.h"
#include "CSerializeBuffer.h"
#include "LockGuard.h"

bool IOCP::Init(String ip, Port port, uint16 backlog, uint16 maxNetThread)
{
	_isRunning = true;
	_maxNetThread = maxNetThread;


	_threadArray = new HANDLE[_maxNetThread+2];
	InitializeSRWLock(&_sessionManagerLock);


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
		_listenSocket.Close();
		return false;
	}

	if (_listenSocket.Listen(backlog)== false)
	{
		_listenSocket.Close();
		return false;
	}

	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (_iocp == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}

	void* _this = this;

	_threadArray[0] = (HANDLE)_beginthreadex(nullptr,0,AcceptEntry,_this, 0, nullptr);
	if (_threadArray[0] == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	printf("AcceptThread %p\n", _threadArray[0]);
	_threadArray[1] = (HANDLE)_beginthreadex(nullptr, 0, MonitorEntry, _this, 0, nullptr);
	if (_threadArray[1] == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	printf("MonitorThread %p\n", _threadArray[1]);
	for (int i = 2; i < maxNetThread+2; ++i)
	{
		_threadArray[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerEntry, _this, 0, nullptr);
		if (_threadArray[i] == INVALID_HANDLE_VALUE)
		{
			_listenSocket.Close();
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
	_listenSocket.Close();
	PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
	auto result = WaitForMultipleObjects(_maxNetThread, _threadArray, true, INFINITE);
	if( result==WAIT_FAILED)
	{
		printf("WaitForMultipleObjects failed %d\n", GetLastError());
	}
	OnEnd();
}

bool IOCP::SendPacket(SessionID sessionId, CSerializeBuffer* buffer)
{
	auto session = getLockedSession(sessionId);
	if (session == nullptr)
		return false;

	UnlockGuard<Session> lock(*session);

	int size = buffer->GetFullSize();
	if (size == 0) 
	{
		DebugBreak();
	}
	session->Enqueue(buffer);
	session->dataNotSend++;
	PostQueuedCompletionStatus(_iocp, -1, (ULONG_PTR)session, &session->_sendExecute._overlapped);

	return true;
}

bool IOCP::DisconnectSession(SessionID sessionId)
{

	AcquireSRWLockExclusive(&_sessionManagerLock);
	auto result =_sessions.find(sessionId);
	if(result==_sessions.end())
	{
		ReleaseSRWLockExclusive(&_sessionManagerLock);
		return false;
	}
	auto session = result->second;
	//_sessions.erase(result);

	ReleaseSRWLockExclusive(&_sessionManagerLock);

	session->Close();

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

void IOCP::WorkerThread(LPVOID arg)
{
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
				OnDisconnect(sessionID);
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
			clientSocket.Close();
			continue;
		}
		_acceptCount++;

		auto newSession = new Session(clientSocket, g_sessionId++, *(IOCP*)(this));

		//아직 안 쓰니까 그냥 수정해도 됨.
		newSession->_refCount = 1;
		RegisterSession(*newSession);

		newSession->RegisterIOCP(_iocp);


		OnConnect(newSession->GetSessionID());
		newSession->_postRecvNotIncrease();


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


void NormalIOCP::RegisterSession(Session& session)
{
	AcquireSRWLockExclusive(&_sessionManagerLock);
	_sessions.insert({ session.GetSessionID(), &session });
	ReleaseSRWLockExclusive(&_sessionManagerLock);
}

bool NormalIOCP::DeleteSession(SessionID id)
{
	AcquireSRWLockExclusive(&_sessionManagerLock);
	auto result = _sessions.erase(id);
	ReleaseSRWLockExclusive(&_sessionManagerLock);
	return result == 1;
}


Session* NormalIOCP::FindSession(uint64 id)
{
	AcquireSRWLockShared(&_sessionManagerLock);
	auto session = _sessions.find(id);
	ReleaseSRWLockShared(&_sessionManagerLock);
	if (session == _sessions.end())
	{
		return nullptr;
	}

	return session->second;
}

Session* NormalIOCP::getLockedSession(SessionID sessionID)
{
	AcquireSRWLockShared(&_sessionManagerLock);
	auto result = _sessions.find(sessionID);
	if (result == _sessions.end())
	{
		ReleaseSRWLockShared(&_sessionManagerLock);
		return nullptr;
	}
	result->second->Lock();
	ReleaseSRWLockShared(&_sessionManagerLock);
	return result->second;
}
