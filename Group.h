#pragma once

#include <chrono>
#include <queue>
#include "Types.h"
#include "Container.h"
#include "TLSLockFreeQueue.h"
#include "BuildOption.h"
#include "Session.h"
class SendBuffer;
class GroupManager;
class GroupExecutable;
class CRecvBuffer;
class IOCP;

namespace GroupInfo 
{
	struct GroupIDHash
	{
		std::size_t operator()(const GroupID& s) const
		{
			return std::hash<unsigned long long>()( s );
		}
	};

	struct GroupIDEqual
	{
		bool operator()(const SessionID& lhs, const SessionID& rhs) const
		{
			return lhs.id == rhs.id;
		}
	};
}



class Group
{
	struct GroupJob 
	{
		enum type
		{
			Recv,
			Enter,
			Leave
		};
		type type;
		SessionID sessionId;
	};


	friend class GroupExecutable;
	friend class GroupManager;
public:

	virtual void OnCreate() {};
	virtual void OnUpdate() {};
	virtual void OnEnter(SessionID id) {};
	virtual void OnLeave(SessionID id) {};
	virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) {};
	virtual ~Group() {};
	GroupID GetGroupID() const {return _groupId;}
	int GetQueued() const { return _jobs.Size();}


	//static constexpr int debugSize = 2001;



	//enum jobType {
	//	Enter,
	//	Leave,
	//	Packet,
	//	Other
	//};
	//struct stDebugData
	//{
	//	SessionID id;
	//	jobType type;
	//	GroupID dst = InvalidGroupID();
	//	String cause;
	//};

	//struct stDebug
	//{
	//	long arrIndex;
	//	stDebugData arr[1000];
	//};
	////세션 id, 종류
	//stDebug debugArr[debugSize];
	////tuple<SessionID, String, thread::id> _debugLeave[debugSize];

	//void Write(SessionID id, jobType type,GroupID dst, String cause)
	//{

	//	auto index = InterlockedIncrement(&debugArr[id.index].arrIndex);

	//	if (type == Leave && debugArr[id.index].arr[index - 1].id != id)
	//		DebugBreak();

	//	debugArr[id.index].arr[index].id = id;
	//	debugArr[id.index].arr[index].type = type;
	//	debugArr[id.index].arr[index].dst = dst;
	//	debugArr[id.index].arr[index].cause = cause;

	//}
protected:
	
	void SendPacket(SessionID id, SendBuffer& buffer) const;
	void SendPackets(SessionID id, vector<SendBuffer>& buffer);
	void MoveSession(SessionID id, GroupID dst) const;
	void LeaveSession(SessionID id, bool update);
	void EnterSession(SessionID id, bool update);
	void SetLoopMs(int loopMS);
	int Sessions() { return _sessions.size(); }
	Group();
	IOCP* _iocp;
	int _loopMs = 10;

	int GetWorkTime() const { return oldWorkTime; }
private:

	bool Enqueue(GroupJob job,bool update = true);

	bool NeedUpdate();
	void Update();
	void Execute(IOCP* iocp) const;
	bool hasJob() const;

	bool HandleJob();

	template <typename Header>
	void RecvHandler(Session& session, void* iocp);


	void onRecvPacket(const Session& session, CRecvBuffer& buffer);
	
	long _leaveCount = 0;
	long _handledLeave = 0;
	long _enterCount = 0;
	long _handledEnter = 0;



private:
	GroupID _groupId = InvalidGroupID();
	GroupExecutable& _executable;
	std::chrono::steady_clock::time_point _nextUpdate;
	SessionSet _sessions;

	LockFreeFixedQueue<GroupJob, 8192> _jobs;
	//TlsLockFreeQueue<GroupJob> _jobs;

	volatile char _isRun = 0;

	GroupManager* _owner;



	long debugIndex = 0;


	//MONITOR
	std::chrono::steady_clock::time_point _nextMonitor;
	int workTime = 0;
	int oldWorkTime = 0;
};

