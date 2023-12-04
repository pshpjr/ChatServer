#include "stdafx.h"
#include "NormalIOCP.h"

#include <chrono>
#include <process.h>

#include "IOCP.h"
#include "Session.h"
#include "CSerializeBuffer.h"

/*****************************/
//			INIT
/*****************************/
IOCP::IOCP()
{
	WSADATA wsaData;
	if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 )
	{
		_isRunning = false;
	}

	for ( int i = MAX_SESSIONS - 1; i >= 0; i-- )
	{
		freeIndex.Push(i);
	}
}


bool IOCP::Init(String ip, Port port, uint16 backlog, uint16 maxNetThread, uint16 workerThread, char staticKey)
{

	
	_isRunning = true;
	_maxNetThread = maxNetThread;
	_staticKey = staticKey;
	_ip = ip;
	_port = port;
	
	if (staticKey) {
		for (auto& session : sessions) {
			session.SetNetSession(staticKey);
			session.SetOwner(*this);
		}
	}

	_threadArray.resize(workerThread);

	_listenSocket.Init();
	if (_listenSocket.isValid() == false)
	{
		return false;
	}
	_listenSocket.setLinger(true);
	_listenSocket.setNoDelay(true);

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

	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, maxNetThread);
	if (_iocp == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}

	void* _this = this;
	for (int i = 0; i < workerThread; ++i)
	{
		_threadArray[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerEntry, _this, 0, nullptr);
		if (_threadArray[i] == INVALID_HANDLE_VALUE)
		{
			_listenSocket.Close();
			return false;
		}
		printf("WorkerThread %p\n", _threadArray[i]);
	}


	auto acceptT = (HANDLE)_beginthreadex(nullptr,0,AcceptEntry,_this, 0, nullptr);
	if (acceptT == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	_threadArray.push_back(acceptT);
	printf("AcceptThread %p\n", acceptT);


	auto monitorT = (HANDLE)_beginthreadex(nullptr, 0, MonitorEntry, _this, 0, nullptr);
	if (monitorT == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	_threadArray.push_back(monitorT);
	printf("MonitorThread %p\n", monitorT);

	auto timeoutT = (HANDLE)_beginthreadex(nullptr, 0, TimeoutEntry, _this, 0, nullptr);
	if (timeoutT == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}

	_threadArray.push_back(timeoutT);
	printf("TimeoutThread %p\n", timeoutT);

	OnInit();
	return true;
}

void IOCP::Start()
{
	InterlockedExchange8(&_isRunning , true);
	WakeByAddressAll(&_isRunning);
	OnStart();
}

void IOCP::Stop()
{
	_isRunning = false;
	_listenSocket.CancleIO();
	PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);

	for (auto h : _threadArray) {
		auto result = WaitForSingleObject(h, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
			printf("WaitForMultipleObjects failed %d %d\n",result, GetLastError());
		}
	}

	gracefulEnd = true;

	OnEnd();
}

void IOCP::SetTimeout(SessionID sessionId, int timeoutMillisecond)
{
	auto result = FindSession(sessionId, L"TimeoutInc");
	if ( result == nullptr )
		return;
	auto& session = *result;

	session.SetTimeout(timeoutMillisecond);

	session.Release(L"setTimeoutRel");
}

void IOCP::SetDefaultTimeout(unsigned int timeoutMillisecond)
{
	if ( timeoutMillisecond == 0 )
	{
		_checkTiemout = false;
		return;
	}

	for ( auto& s : sessions )
	{
		s.SetDefaultTimeout(timeoutMillisecond);
		s.SetTimeout(timeoutMillisecond);
	}
}

/*****************************/
//			Network
/*****************************/

bool IOCP::SendPacket(SessionID sessionId, CSerializeBuffer* buffer, int type)
{
	auto result = FindSession(sessionId, L"SendPacketInc");
	if (result == nullptr)
		return false;
	auto& session = *result;

	session.writeContentLog(type);

	buffer->IncreaseRef(L"SendPacketInc");

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

bool IOCP::SendPacket(SessionID sessionId, CSerializeBuffer* buffer)
{
	auto result = FindSession(sessionId, L"SendPacketInc");
	if (result == nullptr)
		return false;
	auto& session = *result;

	_processBuffer(session, *buffer);

	PostQueuedCompletionStatus(_iocp, -1, (ULONG_PTR)&session, &session._sendExecute._overlapped);
	session.Release(L"SendPacketRelease");
	return true;
}

bool IOCP::SendPackets(SessionID sessionId, list<CSerializeBuffer*>& bufArr)
{
	auto result = FindSession(sessionId, L"SendPacketInc");
	if ( result == nullptr )
		return false;
	auto& session = *result;
	auto len = bufArr.size();
	if ( len == 0 )
		return false;


	for (auto buffer: bufArr) 
	{
		_processBuffer(session, *buffer);
	}

	PostQueuedCompletionStatus(_iocp, -1, ( ULONG_PTR ) &session, &session._sendExecute._overlapped);
	session.Release(L"SendPacketRelease");
	return true;
}


void NormalIOCP::_processBuffer(Session& session, CSerializeBuffer& buffer)
{
	auto result = buffer.IncreaseRef(L"ProcessBuffInc");

	int size = buffer.GetDataSize();
	if ( size == 0 )
	{
		DebugBreak();
	}
	session.Enqueue(&buffer);
	session.dataNotSend++;
}



bool IOCP::DisconnectSession(SessionID sessionId)
{


	auto result = FindSession(sessionId, L"DisconnectInc");
	if (result == nullptr)
		return false;
	auto& session = *result;

	session.Close();

	session.Release(L"DisconnectRel");

	return true;  
}
void IOCP::_onDisconnect(SessionID sessionId)
{
	InterlockedDecrement16(&_sessionCount);
	InterlockedIncrement64(&_disconnectCount);
	OnDisconnect(sessionId);
}

/*****************************/
//			INIT
/*****************************/

bool IOCP::isEnd()
{
	return gracefulEnd;
}

void IOCP::SetMaxPacketSize(int size)
{
	for (auto& session : sessions) {
		session.SetMaxPacketLen(size);
	}
}


/*****************************
			ThreadPool
*****************************/

void IOCP::PostExecutable(Executable* exe, ULONG_PTR arg)
{
	int transfered = 2;
	PostQueuedCompletionStatus(_iocp, transfered, arg, exe->GetOverlapped());

}


void IOCP::SetRecvDebug(SessionID id, unsigned int type)
{
	auto sessionIndex = (id >> 47);
	auto& session = sessions[sessionIndex];

	session.release_D[InterlockedIncrement(&session.debugIndex)%session.debugSize] = { session._refCount,L"RecvPacket",type };
}

/*****************************
			Monitor
*****************************/

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
	return _sessionCount;
}

uint64 IOCP::GetPacketPoolSize()
{
	return _packetPoolSize;
}

uint32 IOCP::GetPacketPoolEmptyCount()
{
	return _packetPoolEmpty;
}

uint64 IOCP::GetTimeoutCount()
{
	return _timeoutSessions;
}

uint32 IOCP::GetPacketPoolAcquireCount()
{
	return CSerializeBuffer::_pool.GetAcquireCount();
}

uint32 IOCP::GetPacketPoolReleaseCount()
{
	return CSerializeBuffer::_pool.GetRelaseCount();
}

uint64 IOCP::GetDisconnectCount()
{
	return _disconnectCount;
}

uint64 IOCP::GetDisconnectPersec()
{
	return _disconnectps;
}


/*****************************
		 ThreadFunc
*****************************/

unsigned __stdcall IOCP::MonitorEntry(LPVOID arg)
{
	IOCP* iocp = ( IOCP* ) arg;

	iocp->MonitorThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::TimeoutEntry(LPVOID arg)
{
	IOCP* iocp = ( IOCP* ) arg;
	iocp->TimeoutThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::WorkerEntry(LPVOID arg)
{
	IOCP* iocp = ( IOCP* ) arg;
	iocp->WorkerThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::AcceptEntry(LPVOID arg)
{
	IOCP* iocp = ( IOCP* ) arg;
	iocp->AcceptThread(nullptr);
	return 0;
}

void IOCP::WorkerThread(LPVOID arg)
{

	srand(GetCurrentThreadId());
	waitStart();
	while (true)
	{
		DWORD transferred = 0;
		Executable* overlapped = nullptr;
		Session* session = nullptr;

		auto ret = GetQueuedCompletionStatus(_iocp, &transferred, ( PULONG_PTR ) &session, ( LPOVERLAPPED* ) &overlapped, INFINITE);

		overlapped = (Executable*)((char*)overlapped - offsetof(Executable, _overlapped));
		if(transferred==0&&session== nullptr)
		{
			PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
			break;
		}

		if(transferred == 0 && session != nullptr)
		{
			auto errNo = WSAGetLastError();

			switch (errNo) {

			case ERROR_NETNAME_DELETED:
				__fallthrough;
			//ERROR_SEM_TIMEOUT. 장치에서 끊은 경우 (5회 재전송 실패 등..)
			case ERROR_SEM_TIMEOUT:
				__fallthrough;
			//WSA_OPERATION_ABORTED cancleIO로 인한 것. 
			case WSA_OPERATION_ABORTED:
				__fallthrough;
				break;

			case ERROR_CONNECTION_ABORTED:
				__fallthrough;
				break;
			default:
				{
					auto now = chrono::system_clock::now();
					auto recvWait = chrono::duration_cast<chrono::milliseconds>(now - session->lastRecv);
					auto sendWait = chrono::duration_cast<chrono::milliseconds>(now - session->_postSendExecute.lastSend);
					DebugBreak();
					int i = 0;
				}
			}

			auto sessionID =session->GetSessionID();
		
			session->Release(L"GQCSerrorRelease", errNo);
		}
		else
		{
			overlapped->Execute((ULONG_PTR)session, transferred, this);
		}

	}
	printf("WorkerThread End\n");
}

//iocp 에서 완료 통지가 올 때 처리는 소켓마다 달라진다 

void IOCP::AcceptThread(LPVOID arg)
{

	waitStart();
	while (_isRunning)
	{
		Socket clientSocket = _listenSocket.Accept();
		if (!clientSocket.isValid()) {
			auto err = WSAGetLastError();
			if ( err == WSAEINTR )
				break;

			printf("AcceptError %d", WSAGetLastError());
			DebugBreak();
			int a = 10;
		}


		if (OnAccept(clientSocket.GetSockAddr()) == false)
		{
			//TODO: 여기서 cancleIO가 의미 없음. 
			clientSocket.CancleIO();
			continue;
		}
		_acceptCount++;

		clientSocket.setLinger(true);
		clientSocket.setNoDelay(true);


		unsigned short sessionIndex;
		if (freeIndex.Pop(sessionIndex) == false) 
		{
			printf("!!!!!serverIsFull!!!!!\n");
			clientSocket.CancleIO();
			continue;
		}

		uint64 sessionID = (uint64)sessionIndex << 47;
		sessionID |= (g_sessionId++);

		auto& session = sessions[sessionIndex];
		auto result = session.IncreaseRef(L"AcceptInc");
		session._recvBuffer = CSerializeBuffer::Alloc();

		session.SetSessionID(sessionID);
		session.SetSocket(clientSocket);
		session.RegisterIOCP(_iocp);

		session.OffReleaseFlag();
		session._connect = true;
		
		OnConnect(session.GetSessionID(),clientSocket.GetSockAddr());
		session.RecvNotIncrease();

		InterlockedIncrement16(&_sessionCount);
	}
	printf("AcceptThreadEnd\n");
}

void IOCP::MonitorThread(LPVOID arg)
{
	waitStart();
	auto start = chrono::system_clock::now();
	auto next = chrono::time_point_cast<chrono::milliseconds>(start) + chrono::milliseconds(1000);
	while (_isRunning)
	{
		_acceptTps = _acceptCount - _oldAccepCount;
		_oldAccepCount = _acceptCount;
		_disconnectps = _disconnectCount - _oldDisconnect;
		_oldDisconnect = _disconnectCount;
		_recvTps = _recvCount;
		_sendTps = _sendCount;
		_packetPoolSize = CSerializeBuffer::_pool.GetGPoolSize();
		_packetPoolEmpty = CSerializeBuffer::_pool.GetGPoolEmptyCount();

		OnMonitorRun();

		InterlockedExchange64(&_recvCount, 0);
		InterlockedExchange64(&_sendCount, 0);

		auto sleepTime = chrono::duration_cast<chrono::milliseconds> (next - chrono::system_clock::now());
		next += chrono::milliseconds(1000);
		if (sleepTime.count() > 0)
			Sleep( (DWORD)sleepTime.count());
	}
	printf("MonitorThread End\n");

}

void IOCP::TimeoutThread(LPVOID arg)
{

	waitStart();
	auto start = chrono::system_clock::now();
	auto next = chrono::time_point_cast<chrono::milliseconds>(start) + chrono::milliseconds(1000);
	while (_isRunning) 
	{
		auto now = chrono::system_clock::now();
		if ( _checkTiemout ) 
		{
			for ( auto& session : sessions )
			{
				if ( session.CheckTimeout(now) ) 
				{
					OnSessionTimeout(session.GetSessionID(),session.GetIP(),session.GetPort());
					session.TimeoutReset();
					_timeoutSessions++;
				}
			}
		}

		auto sleepTime = chrono::duration_cast<chrono::milliseconds> (next - chrono::system_clock::now());
		next += chrono::milliseconds(1000);
		if (sleepTime.count() > 0)
			Sleep((DWORD)sleepTime.count());
	}
	printf("TimeoutThread End\n");

}

void IOCP::increaseRecvCount()
{
	InterlockedIncrement64(&_recvCount);
}



NormalIOCP::~NormalIOCP()
{
	CloseHandle(_iocp);
	for (auto h : _threadArray)
	{
		printf("CloseHandle %p\n", h);

		CloseHandle(h);
	}
}


Session* NormalIOCP::FindSession(uint64 id, LPCWSTR content)
{
	auto sessionIndex = id >> 47;
	auto& session = sessions[sessionIndex];
	auto result = session.IncreaseRef(content);

	//릴리즈 중이거나 세션 변경되었으면 
	if (result > releaseFlag || session.GetSessionID() != id)
	{
		//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
		session.Release(L"sessionChangedRelease");
		return nullptr;
	}

	return &session;
}

void NormalIOCP::waitStart()
{
	char unDesire = false;
	char cur = _isRunning;
	while ( unDesire == cur )
	{
		WaitOnAddress(&_isRunning, &unDesire, sizeof(char), INFINITE);
		cur = _isRunning;
	}

}