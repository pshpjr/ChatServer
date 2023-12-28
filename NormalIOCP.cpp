#include "stdafx.h"
#include "NormalIOCP.h"
#include <synchapi.h>
#pragma comment(lib,"Synchronization.lib")
#pragma comment(lib,"Winmm.lib")
//#define BUILD_WITH_EASY_PROFILER
#include "externalHeader/easy/profiler.h"



#include <chrono>
#include <format>




#include "IOCP.h"
#include "Session.h"
#include "CSendBuffer.h"
#include "CRecvBuffer.h"
#include "SingleThreadQ.h"

#include "CLogger.h"
#include "CoreGlobal.h"
#include "Executable.h"
#include "Executables.h"
#include "SettingParser.h"
#include "SettingParser.h"

#include "Client.h"
/*****************************/
//			INIT
/*****************************/




IOCP::IOCP()
{
	timeBeginPeriod(1);
	WSADATA wsaData;
	if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 )
	{
		_isRunning = false;
	}

	for ( int i = MAX_SESSIONS - 1; i >= 0; i-- )
	{
		freeIndex.Push(i);
	}
	_groupManager = new GroupManager(this);

	_settingParser.Init();
	String ip;
	Port port;
	uint16 backlog;
	uint16 maxNetThead;
	uint16 workerThread;
	char staticKey;

	_settingParser.GetValue(L"basic.ip", ip);
	_settingParser.GetValue(L"basic.port", port);
	_settingParser.GetValue(L"basic.backlog", backlog);
	_settingParser.GetValue(L"basic.runningThreads", maxNetThead);
	_settingParser.GetValue(L"basic.workerThreads", workerThread);
	_settingParser.GetValue(L"basic.staticKey", staticKey);

	Init(ip, port, backlog, maxNetThead, workerThread, staticKey);

}


bool IOCP::Init(const String& ip, const Port port, const uint16 backlog, const uint16 maxRunningThread, const uint16 workerThread, const char staticKey)
{

	_maxNetThread = maxRunningThread;
	_staticKey = staticKey;
	_ip = ip;
	_port = port;
	
	for ( auto& session : _sessions )
	{
		session.SetOwner(*this);
	}

	_threadArray.resize(workerThread);

	_listenSocket.Init();
	if (_listenSocket.IsValid() == false)
	{
		return false;
	}
	_listenSocket.SetLinger(true);
	//_listenSocket.setNoDelay(true);
	//_listenSocket.setSendbuffer(0);
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

	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, maxRunningThread);
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


	const auto acceptT = (HANDLE)_beginthreadex(nullptr,0,AcceptEntry,_this, 0, nullptr);
	if (acceptT == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	_threadArray.push_back(acceptT);
	printf("AcceptThread %p\n", acceptT);


	const auto timeoutT = ( HANDLE ) _beginthreadex(nullptr, 0, TimeoutEntry, _this, 0, nullptr);
	if ( timeoutT == INVALID_HANDLE_VALUE )
	{
		_listenSocket.Close();
		return false;
	}


	const auto monitorT = (HANDLE)_beginthreadex(nullptr, 0, MonitorEntry, _this, 0, nullptr);
	if (monitorT == INVALID_HANDLE_VALUE)
	{
		_listenSocket.Close();
		return false;
	}
	
	_threadArray.push_back(monitorT);
	printf("MonitorThread %p\n", monitorT);


	_threadArray.push_back(timeoutT);
	printf("TimeoutThread %p\n", timeoutT);


	for ( auto thread : _threadArray )
	{
		_threadMonitors.emplace_back(thread);
	}

	OnInit();
	return true;
}

void IOCP::Start()
{
	OnStart();
	InterlockedExchange8(&_isRunning , true);
	WakeByAddressAll(&_isRunning);

}

void IOCP::Stop()
{
	_isRunning = false;
	_listenSocket.Close();
	PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);

	for (const auto h : _threadArray) {
		if (const auto result = WaitForSingleObject(h, INFINITE); result != WAIT_OBJECT_0)
		{
			printf("WaitForMultipleObjects failed %d %d\n",result, GetLastError());
		}
	}

	InterlockedExchange8(&gracefulEnd, true);
	OnEnd();
	WakeByAddressSingle(&gracefulEnd);
}

void IOCP::Wait()
{
	char unExpect = false;
	char capture = gracefulEnd;

	while ( capture == unExpect )
	{
		WaitOnAddress(&gracefulEnd, &unExpect, sizeof(unExpect), INFINITE);
		capture = gracefulEnd;
	}

}

void IOCP::SetTimeout(const SessionID sessionId, const int timeoutMillisecond)
{
	const auto result = FindSession(sessionId, L"TimeoutInc");
	if ( result == nullptr )
	{
		return;
	}
	auto& session = *result;

	session.SetTimeout(timeoutMillisecond);

	session.Release(L"setTimeoutRel");
}

void IOCP::SetDefaultTimeout(const unsigned int timeoutMillisecond)
{
	if ( timeoutMillisecond == 0 )
	{
		_checkTiemout = false;
		return;
	}

	for ( auto& s : _sessions )
	{
		s.SetDefaultTimeout(timeoutMillisecond);
		s.SetTimeout(timeoutMillisecond);
	}
}

/*****************************/
//			Network
/*****************************/

bool IOCP::SendPacket(const SessionID sessionId, CSendBuffer* buffer, const int type)
{
	EASY_FUNCTION();

	const auto result = FindSession(sessionId, L"SendPacketInc");
	if ( result == nullptr )
	{
		return false;
	}
		
	auto& session = *result;

	session.WriteContentLog(type);

	ProcessBuffer(session, *buffer);


	//session.trySend();

	if ( session.CanSend())
	{
		//InterlockedIncrement(&_iocpCompBufferSize);
		EASY_BLOCK("PQCS");
		PostQueuedCompletionStatus(_iocp, -1, ( ULONG_PTR ) &session, &session._sendExecute._overlapped);
	}

	session.Release(L"SendPacketRelease");
	return true;
}


bool IOCP::SendPackets(const SessionID sessionId, SingleThreadQ<CSendBuffer*>& bufArr)
{
	const auto result = FindSession(sessionId, L"SendPacketInc");
	if ( result == nullptr )
	{
		return false;
	}
	auto& session = *result;

	CSendBuffer* buffer;
	while ( bufArr.Dequeue(buffer) )
	{
		ProcessBuffer(session, *buffer);
		buffer->Release(L"SendPacketsRelease");
	}

	if ( session._isSending == 0 )
	{
		PostQueuedCompletionStatus(_iocp, -1, ( ULONG_PTR ) &session, &session._sendExecute._overlapped);
	}
	session.Release(L"SendPacketRelease");
	return true;
}



void NormalIOCP::ProcessBuffer(Session& session, CSendBuffer& buffer)
{
	buffer.IncreaseRef(L"ProcessBuffInc");

	if ( buffer.GetDataSize() == 0 )
	{
		DebugBreak();
	}
	session.EnqueueSendData(&buffer);
}

WSAResult<SessionID>  IOCP::GetClientSession(const String& ip, const Port port)
{
	unsigned short index;
	freeIndex.Pop(index);

	Socket s;
	s.Init();

	if ( !s.IsValid() )
	{
		return s.LastError();
	}

	s.SetLinger(true);
	s.SetNoDelay(true);

	if (auto connectionResult = s.Connect(ip, port); !connectionResult.HasValue() )
	{
		s.Close();
		freeIndex.Push(index);
		
		return connectionResult.Error();
	}

	_sessions[index].SetSocket(s);

	SessionID sessionId;

	//클라이언트 사용이 많지 않고, 계속 재접속 할 거 아니고, 보통 accept 전에 하니까 그냥 풀어둬도 될 것 같음. 
	//인덱스는 안 겹침. 
	sessionId.id = ++g_sessionId;
	sessionId.index = index;

	_sessions[index].IncreaseRef(L"AcceptInc");
	_sessions[index].SetSessionId(sessionId);
	_sessions[index].RegisterIOCP(_iocp);
	_sessions[index].OffReleaseFlag();
	_sessions[index].SetConnect();
	_sessions[index].SetLanSession();

	_sessions[index].RecvNotIncrease();
	
	return sessionId;
}

bool IOCP::IsValidSession(const SessionID id)
{
	const auto findResult = FindSession(id);
	if ( findResult != nullptr )
	{
		findResult->Release(L"validSessionRelease");
	}

	return nullptr != findResult;
}


void IOCP::SetDisableClickAndClose()
{
	DWORD consoleModePrev;
	const HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(handle, &consoleModePrev);
	SetConsoleMode(handle, consoleModePrev & ~ENABLE_QUICK_EDIT_MODE);

	// get the handle to the console
	const HWND consoleWindow = GetConsoleWindow();

	// get handle to the System Menu
	const HMENU systemMenu = GetSystemMenu(consoleWindow, 0);

	// how many items are there?
	const int itemCount = GetMenuItemCount(systemMenu);
	int j = -1;
	const auto buf = static_cast<WCHAR*>(malloc(256 * sizeof(WCHAR)));

	for ( int i = 0; i < itemCount; i++ )
	{
		// find the one we want
		GetMenuStringW(systemMenu, i, buf, 255, MF_BYPOSITION);
		if ( !wcscmp(buf, L"닫기(&C)") )
		{
			j = i;
			break;
		}
	}

	// if found, remove that menu item
	if ( j >= 0 )
	{
		RemoveMenu(systemMenu, j, MF_BYPOSITION);
	}
}

bool IOCP::DisconnectSession(const SessionID sessionId)
{
	const auto result = FindSession(sessionId, L"DisconnectInc");
	if (result == nullptr)
	{
		return false;
	}
	auto& session = *result;

	session.Close();

	session.Release(L"DisconnectRel");

	return true;  
}

void IOCP::_onDisconnect(const SessionID sessionId)
{
	InterlockedDecrement16(&_sessionCount);
	InterlockedIncrement64(&_disconnectCount);

	OnDisconnect(sessionId);
}

void IOCP::onRecvPacket(const Session& session, CRecvBuffer& buffer)
{
	try
	{
		OnRecvPacket(session._sessionId, buffer);
	}
	catch ( const std::invalid_argument& )
	{
		gLogger->Write(L"RecvErr", LogLevel::Debug, L"ERR");
	}
}

/*****************************/
//			INIT
/*****************************/

bool IOCP::isEnd() const
{
	return gracefulEnd;
}

void IOCP::SetMaxPacketSize(const int size) 
{
	for (auto& session : _sessions) {
		session.SetMaxPacketLen(size);
	}
}


/*****************************
			ThreadPool
*****************************/

void IOCP::PostExecutable(Executable* exe, const ULONG_PTR arg) const
{
	constexpr int transferred = 2;
	PostQueuedCompletionStatus(_iocp, transferred, arg, exe->GetOverlapped());
}

void IOCP::SetSessionStaticKey(const SessionID id, const char staticKey)
{
	const auto result = FindSession(id, L"setStaticKey");
	if ( result == nullptr )
	{
		return;
	}
	auto& session = *result;

	session.SetNetSession(staticKey);
	session.Release();
}


/*****************************
			Monitor
*****************************/

uint64 IOCP::GetAcceptCount() const
{
	return _acceptCount;
}

uint64 IOCP::GetAcceptTps() const
{
	return _acceptTps;
}

uint64 IOCP::GetRecvTps() const
{
	return _recvTps;
}

uint64 IOCP::GetSendTps() const
{
	return _sendTps;
}

uint16 IOCP::GetSessions() const
{
	return _sessionCount;
}

uint64 IOCP::GetPacketPoolSize() const
{
	return _packetPoolSize;
}

uint32 IOCP::GetPacketPoolEmptyCount() const
{
	return _packetPoolEmpty;
}

uint64 IOCP::GetTimeoutCount() const
{
	return _timeoutSessions;
}

uint32 IOCP::GetPacketPoolAcquireCount()
{
	return CSendBuffer::_pool.GetAcquireCount();
}

uint32 IOCP::GetPacketPoolReleaseCount()
{
	return CSendBuffer::_pool.GetReleaseCount();
}

uint64 IOCP::GetDisconnectCount() const
{
	return _disconnectCount;
}

uint64 IOCP::GetDisconnectPerSec() const
{
	return _disconnectps;
}

uint32 IOCP::GetSegmentTimeout() const
{
	return _tcpSegmenTimeout;
}

size_t IOCP::GetPageFaultCount() const
{
	return _memMonitor.GetPageFaultCount();
}

size_t IOCP::GetPeakPMemSize() const
{
	return _memMonitor.GetPeakPMemSize();
}

size_t IOCP::GetPMemSize() const
{
	return _memMonitor.GetPMemSize();
}

size_t IOCP::GetPeakVirtualMemSize() const
{
	return _memMonitor.GetPeakVirtualMemSize();
}

size_t IOCP::GetVirtualMemSize() const
{
	return _memMonitor.GetVirtualMemSize();
}

size_t IOCP::GetPeakPagedPoolUsage() const
{
	return _memMonitor.GetPeakPagedPoolUsage();
}

size_t IOCP::GetPeakNonPagedPoolUsage() const
{
	return _memMonitor.GetPeakNonPagedPoolUsage();
}

size_t IOCP::GetPagedPoolUsage() const
{
	return _memMonitor.GetPagedPoolUsage();
}

size_t IOCP::GetNonPagedPoolUsage() const
{
	return _memMonitor.GetNonPagedPoolUsage();
}

String IOCP::GetLibMonitorString() const
{

	return std::format(L"==================================================================================\n"
				L" {:<11s}{:^55s}{:>11s}\n"
				L"----------------------------------------------------------------------------------\n"
				L"+  {:<14s} : {:>6.2f}MB / {:<6.2f}MB | {:<14s} : {:>6.2f}MB / {:<6.2f}MB \n"
				L"+  {:<14s} : {:>6.2f}%% / {:<6.2f}%%  |\n"
				L"+  {:<14s} : {:>6.2f}%% / {:<6.2f}%%  |  {:<14s} : {:>6.2f}%% / {:<6.2f}%%  \n"
				L"----------------------------------------------------------------------------------\n"
				L"+  {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"+  {:<14s} : {:>10d}, {:<14s} : {:>10d}\n"
				L"----------------------------------------------------------------------------------\n"
				, L"+++++", L"LIBS", L"+++++"
				, L"VirtualMem", GetVirtualMemSize() / 1000'000., GetPeakVirtualMemSize() / 1000'000.
				, L"PhysicalMem", GetPMemSize() / 1000'000., GetPeakPMemSize() / 1000'000.
				, L"TotalCPU", _cpuMonitor.ProcessTotal(), _cpuMonitor.ProcessorTotal()
				, L"KernelCPU", _cpuMonitor.ProcessKernel(), _cpuMonitor.ProcessorKernel()
				, L"UserCPU", _cpuMonitor.ProcessUser(), _cpuMonitor.ProcessorUser()
				, L"Sessions", GetSessions()
				, L"Accept", GetAcceptCount(), L"AcceptTPS", GetAcceptTps()
				, L"RecvTPS", GetRecvTps(), L"SendTPS", GetSendTps()
				, L"Disconnect", GetDisconnectCount(), L"DisconnectTPS", GetDisconnectPerSec()
				, L"SegmentTimeout", GetSegmentTimeout(), L"Timeout", GetTimeoutCount()
				, L"GSendPool", CSendBuffer::_pool.GetGPoolSize(), L"GSendPoolEmpty", CSendBuffer::_pool.GetGPoolEmptyCount()
				, L"GRecvPool", CRecvBuffer::_pool.GetGPoolSize(), L"GRecvPoolEmpty", CRecvBuffer::_pool.GetGPoolEmptyCount()
				, L"SendPoolAcq", CSendBuffer::_pool.GetAcquireCount(), L"SendPoolRel", CSendBuffer::_pool.GetReleaseCount()
				, L"RecvPoolAcq", CRecvBuffer::_pool.GetAcquireCount(), L"RecvPoolRel", CRecvBuffer::_pool.GetReleaseCount()
			     ,L"SendPoolGap", CSendBuffer::_pool.GetAcquireCount()- CSendBuffer::_pool.GetReleaseCount(),L"RecvPoolGap", CRecvBuffer::_pool.GetAcquireCount() - CRecvBuffer::_pool.GetReleaseCount()

	);


}

void IOCP::PrintMonitorString() const
{
	const auto tmpS = GetLibMonitorString();

	wprintf_s(tmpS.c_str());

}


/*****************************
		 ThreadFunc
*****************************/

unsigned __stdcall IOCP::MonitorEntry(const LPVOID arg)
{
	const auto iocp = static_cast<IOCP*>(arg);

	iocp->MonitorThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::TimeoutEntry(const LPVOID arg)
{
	const auto iocp = static_cast<IOCP*>(arg);
	iocp->TimeoutThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::WorkerEntry(const LPVOID arg)
{
	const auto iocp = static_cast<IOCP*>(arg);
	iocp->WorkerThread(nullptr);
	return 0;
}

unsigned __stdcall IOCP::AcceptEntry(const LPVOID arg)
{
	const auto iocp = static_cast<IOCP*>(arg);
	iocp->AcceptThread(nullptr);
	return 0;
}

void IOCP::WorkerThread(LPVOID arg)
{
	srand(GetCurrentThreadId());
	WaitStart();
	EASY_THREAD("WORKER");
	while (true)
	{

		bool end = false;
		OVERLAPPED_ENTRY arr[50] = { 0, };
		ULONG size = 0;

		{
			EASY_BLOCK("GQCS");
			auto ret = GetQueuedCompletionStatusEx(_iocp, arr, 50, &size, INFINITE, false);
		}
		
		EASY_BLOCK("EXECUTE");
		for ( unsigned int i = 0; i < size; i++ )
		{
			const DWORD transferred = arr[i].dwNumberOfBytesTransferred;
			const LPOVERLAPPED overlap = arr[i].lpOverlapped;
			auto session = ( Session* )arr[i].lpCompletionKey;

			Executable* overlapped = Executable::GetExecutable(overlap);
			if ( transferred == 0 && session == nullptr )
			{
				PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
				end = true;

				break;
			}

			if ( transferred == 0 && session != nullptr )
			{
				const auto errNo = WSAGetLastError();
				switch ( errNo )
				{
					//정상종료됨. 
					case 0:
						break;
					case ERROR_NETNAME_DELETED:
						break;
						//ERROR_SEM_TIMEOUT. 장치에서 끊은 경우 (5회 재전송 실패 등..)
					case ERROR_SEM_TIMEOUT:
						InterlockedIncrement(&_tcpSegmenTimeout);
						[[fallthrough]];
						//WSA_OPERATION_ABORTED cancleIO로 인한 것. 
					case WSA_OPERATION_ABORTED:
						[[fallthrough]];
					case WSA_IO_PENDING:
						[[fallthrough]];
					case ERROR_CONNECTION_ABORTED:
						[[fallthrough]];
					case ERROR_NOT_FOUND:
						[[fallthrough]];
					case WSAECONNRESET:
						break;
					[[unlikely]] default:
					{
						auto now = chrono::system_clock::now();
						auto recvWait = chrono::duration_cast< chrono::milliseconds >( now - session->lastRecv );
						auto sendWait = chrono::duration_cast< chrono::milliseconds >( now - session->_postSendExecute.lastSend );
						DebugBreak();
						int tmp = 0;
					}
				}


				auto sessionID = session->GetSessionId();

				session->Release(L"GQCSErrorRelease", errNo);
			}
			else
			{
				overlapped->Execute(( ULONG_PTR ) session, transferred, this);
			}
	

		}
		if ( end )
		{
			break;
		}

		//DWORD transferred = 0;
		//LPOVERLAPPED overlap = nullptr;
		//Session* session = nullptr;

		//auto ret = GetQueuedCompletionStatus(_iocp, &transferred, ( PULONG_PTR ) &session, &overlap, INFINITE);
	
		//Executable* overlapped = Executable::GetExecutable(overlap);
		//if(transferred==0&&session== nullptr)
		//{
		//	PostQueuedCompletionStatus(_iocp, 0, 0, nullptr);
		//	break;
		//}

		//if(transferred == 0 && session != nullptr)
		//{
		//	auto errNo = WSAGetLastError();
		//	switch (errNo) {
		//	//정상종료됨. 
		//	case 0:
		//		break;
		//	case ERROR_NETNAME_DELETED:
		//		break;
		//	//ERROR_SEM_TIMEOUT. 장치에서 끊은 경우 (5회 재전송 실패 등..)
		//	case ERROR_SEM_TIMEOUT:
		//		InterlockedIncrement(&_tcpSegmenTimeout);
		//		[[fallthrough]];
		//	//WSA_OPERATION_ABORTED cancleIO로 인한 것. 
		//	case WSA_OPERATION_ABORTED:
		//		break;
		//	case WSA_IO_PENDING:
		//		break;
		//	case ERROR_CONNECTION_ABORTED:
		//		break;
		//	case ERROR_NOT_FOUND:
		//		break;
		//	default:
		//		{
		//			auto now = chrono::system_clock::now();
		//			auto recvWait = chrono::duration_cast<chrono::milliseconds>(now - session->lastRecv);
		//			auto sendWait = chrono::duration_cast<chrono::milliseconds>(now - session->_postSendExecute._lastSend);
		//			DebugBreak();
		//			int i = 0;
		//		}
		//	}

		//	auto sessionID =session->GetSessionID();
		//
		//	session->Release(L"GQCSErrorRelease", errNo);
		//}
		//else
		//{
		//	overlapped->Execute((ULONG_PTR)session, transferred, this);
		//}

	}
	printf("WorkerThread End\n");
}

//iocp 에서 완료 통지가 올 때 처리는 소켓마다 달라진다 

void IOCP::AcceptThread(LPVOID arg)
{

	WaitStart();
	EASY_THREAD("ACCEPT");
	while (_isRunning)
	{
		Socket clientSocket = _listenSocket.Accept();
		EASY_BLOCK("ACCEPT")
		if (!clientSocket.IsValid()) {
			//accept 서버 닫으려고 중단한 경우. 
			if (const auto err = WSAGetLastError(); err == WSAEINTR )
			{
				break;
			}

			printf("AcceptError %d", WSAGetLastError());
			_acceptErrorCount++;
			DebugBreak();
			int a = 10;
		}

		if (OnAccept(clientSocket.GetSockAddress()) == false)
		{
			//TODO: 여기서 cancleIO가 의미 없음. 
			clientSocket.CancelIO();
			continue;
		}
		_acceptCount++;

		
		clientSocket.SetLinger(true);
		//clientSocket.setNoDelay(true);
		//clientSocket.setSendbuffer(0);

		unsigned short sessionIndex;
		if (freeIndex.Pop(sessionIndex) == false) 
		{
			printf("!!!!!serverIsFull!!!!!\n");
			clientSocket.CancelIO();
			continue;
		}

		SessionID sessionID {};
		sessionID.id = ++g_sessionId;
		sessionID.index = sessionIndex;

		auto& session = _sessions[sessionIndex];

		auto result = session.IncreaseRef(L"AcceptInc");

		session.SetSessionId(sessionID);
		session.SetSocket(clientSocket);
		session.RegisterIOCP(_iocp);
		session.SetNetSession(_staticKey);
		session.OffReleaseFlag();
		session._connect = true;
		
		OnConnect(session.GetSessionId(),clientSocket.GetSockAddress());
		session.RecvNotIncrease();

		InterlockedIncrement16(&_sessionCount);
	}
	printf("AcceptThreadEnd\n");
}

void IOCP::MonitorThread(LPVOID arg)
{
	WaitStart();
	EASY_THREAD("MONITOR");
	const auto start = chrono::system_clock::now();
	auto next = start + 1s;
	while (_isRunning)
	{
		{
				_acceptTps = _acceptCount - _oldAccepCount;
			_oldAccepCount = _acceptCount;
			_disconnectps = _disconnectCount - _oldDisconnect;
			_oldDisconnect = _disconnectCount;
			_recvTps = InterlockedExchange64(&_recvCount, 0);
			_sendTps = InterlockedExchange64(&_sendCount, 0);
			_packetPoolSize = CSendBuffer::_pool.GetGPoolSize();
			_packetPoolEmpty = CSendBuffer::_pool.GetGPoolEmptyCount();

			_memMonitor.Update();
			_cpuMonitor.UpdateCpuTime();

			OnMonitorRun();
		}

		
		auto sleepTime = chrono::duration_cast<chrono::milliseconds> (next - chrono::system_clock::now());
		next += 1s;
		if ( sleepTime.count() > 0 )
		{
			Sleep(static_cast<DWORD>(sleepTime.count()));
		}
			
	}
	printf("MonitorThread End\n");

}

void IOCP::TimeoutThread(LPVOID arg)
{

	WaitStart();
	const auto start = chrono::system_clock::now();
	auto next = chrono::time_point_cast<chrono::milliseconds>(start) + chrono::milliseconds(1000);
	while (_isRunning) 
	{
		const auto now = chrono::system_clock::now();

		EASY_BLOCK("TIMEOUT");
		if ( _checkTiemout ) 
		{
			for ( auto& session : _sessions )
			{
				if ( !session.CheckTimeout(now) )
				{
					continue;
				}

				OnSessionTimeout(session.GetSessionId());
				session.ResetTimeoutWait();
				_timeoutSessions++;
			}
		}
		EASY_END_BLOCK;
		auto sleepTime = chrono::duration_cast<chrono::milliseconds> (next - chrono::system_clock::now());
		next += chrono::milliseconds(1000);
		if (sleepTime.count() > 0)
		{
			Sleep(static_cast<DWORD>(sleepTime.count()));
		}
	}
	printf("TimeoutThread End\n");

}

void IOCP::IncreaseRecvCount(const int value)
{
	thread_local int localRecvCount;

	for ( int i = 0; i < value; i++ )
	{
		++localRecvCount;
		if ( localRecvCount == 100 )
		{
			InterlockedAdd64(&_recvCount, 100);
			localRecvCount = 0;
		}

	}

}

void IOCP::increaseSendCount(const int value)
{
	thread_local int localSendCount;
	for ( int i = 0; i < value; i++ )
	{
		++localSendCount;
		if ( localSendCount == 100 )
		{
			InterlockedAdd64(&_sendCount, 100);
			localSendCount = 0;
		}
	}
}

void IOCP::MoveSession(const SessionID target, const GroupID dst) const
{
	_groupManager->MoveSession(target, dst);
}



NormalIOCP::~NormalIOCP()
{
	CloseHandle(_iocp);
	for (const auto h : _threadArray)
	{
		printf("CloseHandle %p\n", h);

		CloseHandle(h);
	}
}


NormalIOCP::NormalIOCP() :_port(0), _groupManager(nullptr), _settingParser(*new SettingParser()) {}

optional<Session*> NormalIOCP::findSession(const SessionID id, const LPCWSTR content)
{
	auto& session = _sessions[id.index];
	const auto result = session.IncreaseRef(content);

	//릴리즈 중이거나 세션 변경되었으면 
	if (result > releaseFlag || session.GetSessionId().id != id.id)
	{
		//세션 릴리즈 해도 문제 없음. 플레그 서 있을거라 내가 올린 만큼 내려감. 
		session.Release(L"sessionChangedRelease");
		return {};
	}

	return &session;
};

void NormalIOCP::WaitStart()
{
	char unDesire = false;
	char cur = _isRunning;
	while ( unDesire == cur )
	{
		WaitOnAddress(&_isRunning, &unDesire, sizeof(char), INFINITE);
		cur = _isRunning;
	}

}


