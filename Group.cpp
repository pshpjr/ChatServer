#include "stdafx.h"
#include "Group.h"
#include "IOCP.h"
#include "CRecvBuffer.h"
#include "GroupExecutable.h"
#include "GroupManager.h"
#include <Protocol.h>
#include <CLogger.h>
#include "CoreGlobal.h"
#include "Profiler.h"

Group::Group() :_iocp(nullptr), _executable(*new GroupExecutable(this)), _nextUpdate(std::chrono::steady_clock::now() + chrono::milliseconds(_loopMs))
, _owner(nullptr) {};

bool Group::Enqueue(GroupJob job, bool update)
{
	_jobs.Enqueue(job);
	
	if (update && _isRun == 0)
	{
		Update();
		return true;
	}
	return false;
}

bool Group::NeedUpdate()
{
	return ::chrono::steady_clock::now() >= _nextUpdate;
}

void Group::Update()
{
	if (InterlockedExchange8(&_isRun, 1) == 1)
		return;
	
	auto start = chrono::steady_clock::now();


	while (hasJob() || NeedUpdate())
	{
		HandleJob();

		if (NeedUpdate())
		{
			OnUpdate();
			_nextUpdate += chrono::milliseconds(_loopMs);
		}

		auto end = chrono::steady_clock::now();

		workTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		if (end > _nextMonitor)
		{
			oldWorkTime = workTime;
			workTime = 0;
			_nextMonitor += 1s;
		}
	}

	InterlockedExchange8(&_isRun, 0);
}


bool Group::HandleJob()
{
	GroupJob job;
	while (_jobs.Dequeue(job))
	{
		switch (job.type) 
		{
		case Group::GroupJob::type::Enter:
		{
			PRO_BEGIN(L"ENTER");
			++_handledEnter;

			ASSERT_CRASH(_sessions.find(job.sessionId) == _sessions.end());
			//Write(job.sessionId, Group::jobType::Enter, InvalidGroupID(), L"Enter Handled");
			_sessions.insert(job.sessionId);

			OnEnter(job.sessionId);
		}
			break;
		case Group::GroupJob::type::Leave:
		{
			PRO_BEGIN(L"LEAVE");
			++_handledLeave;
			//Write(job.sessionId, Group::jobType::Leave, InvalidGroupID(), L"Leave Handled");
			if (_sessions.erase(job.sessionId) == 0)
			{
				DebugBreak();
			}
			ASSERT_CRASH(_sessions.size() == _handledEnter - _handledLeave);
			OnLeave(job.sessionId);

			_iocp->CheckPostRelease(job.sessionId, _groupId);
		}
			break;
		case Group::GroupJob::type::Recv:
		{
			PRO_BEGIN(L"RECV");
			Session* session;

			session = _iocp->FindSession(job.sessionId, L"HandlePacket");
			
			if (session == nullptr)
			{
				//Write(job.sessionId, Group::jobType::Other, InvalidGroupID(), L"InvalidSession when recv");
				continue;
			}

			if (const char sKey = session->_staticKey)
			{
				RecvHandler<NetHeader>(*session, _iocp);
			}
			else
			{
				RecvHandler<LANHeader>(*session, _iocp);
			}

			session->Release(L"HandlePacketRelease");
		}
			break;
		}
	}
	return true;
}

void Group::onRecvPacket(const Session& session, CRecvBuffer& buffer)
{
	try
	{
		OnRecv(session._sessionId, buffer);
		ASSERT_CRASH(buffer.CanPopSize() == 0, "can pop from recv buffer");
	}
	catch (const std::invalid_argument&)
	{
		_iocp->DisconnectSession(session.GetSessionId());
		gLogger->Write(L"Recv Pop Err",CLogger::LogLevel::Debug, L"ERR");
	}
}


//해당 세션이 언제 나갈지 모르는데 상관 없나?
//어짜피 동시에 처리되는 건 아니니까 상관 x.
void Group::LeaveSession(const SessionID id, bool update)
{ 
	Enqueue({ Group::GroupJob::type::Leave, id },update);
}

void Group::EnterSession(const SessionID id, bool update)
{
	Enqueue({ Group::GroupJob::type::Enter, id }, update);
}

void Group::SetLoopMs(int loopMS)
{
	_loopMs = loopMS;
}

void Group::SendPacket(const SessionID id, SendBuffer& buffer) const
{
	_iocp->SendPacket(id, buffer);
}

void Group::SendPackets(SessionID id, vector<SendBuffer>& buffer)
{
	_iocp->SendPackets(id, buffer);
}

void Group::MoveSession(const SessionID id, const GroupID dst) const
{
	_owner->MoveSession(id, dst,false);
}

void Group::Execute(IOCP* iocp) const
{
	_executable.Execute(0, executable::ExecutableTransfer::GROUP, iocp);
}

bool Group::hasJob() const
{
	return _jobs.Size() > 0;
}




template <typename Header>
void Group::RecvHandler(Session& session, void* iocp)
{
	IOCP& server = *static_cast<IOCP*>(iocp);
	int loopCount = 0;

	CRingBuffer& recvQ = session._recvQ;
	while (true)
	{
		if (session.GetGroupID() != _groupId)
		{
			//Write(session.GetSessionId(), Other, session.GetGroupID(), L"Not this Group");
			break;
		}

		if (recvQ.Size() < sizeof(Header))
		{
			break;
		}


		Header header;
		recvQ.Peek((char*)&header, sizeof(Header));

		if constexpr (is_same_v<Header, NetHeader>)
		{
			if (header.code != dfPACKET_CODE)
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				SessionID id = session.GetSessionId();
				gLogger->Write(L"Disconnect",CLogger::LogLevel::Debug, L"Group WrongPacketCode, SessionID : %d\n frontIndex: %d \n %s ",id, frontIndex, dump);
				session.Close();
				break;
			}

			if (header.len > session._maxPacketLen)
			{
				int frontIndex = recvQ.GetFrontIndex();
				std::string dump(recvQ.GetBuffer(), recvQ.GetBuffer() + recvQ.GetBufferSize());
				SessionID id = session.GetSessionId();
				gLogger->Write(L"Disconnect",CLogger::LogLevel::Debug, L"Group WrongPacketLen, SessionID : %d\n index: %d \n %s",id, frontIndex, dump);
				session.Close();
				break;
			}
		}

		if (const int totPacketSize = header.len + sizeof(Header); recvQ.Size() < totPacketSize)
		{
			break;
		}

		CRecvBuffer buffer(&recvQ, header.len);
		if constexpr (is_same_v<Header, NetHeader>)
		{
			recvQ.Decode(session._staticKey);
			
			if (!recvQ.ChecksumValid())
			{
				
				gLogger->Write(L"Disconnect", CLogger::LogLevel::Debug, L"Group Invalid Checksum %d",session.GetSessionId());
				session.Close();
				break;
			}
		}
		loopCount++;
		recvQ.Dequeue(sizeof(Header));
		onRecvPacket(session, buffer);
	}

	if (loopCount > 0)
	{
		server.IncreaseRecvCount(loopCount);
	}
}
