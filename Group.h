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
			recv,
			update
		};
		type type;
		SessionID sessionid;
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


protected:
	void SendPacket(SessionID id, SendBuffer& buffer) const;
	void SendPackets(SessionID id, vector<SendBuffer>& buffer);
	void MoveSession(SessionID id, GroupID dst) const;
	void LeaveSession(SessionID id,String cause);
	void EnterSession(SessionID id);
	void SetLoopMs(int loopMS);
	int Sessions() { return _sessions.size(); }
	Group();
	IOCP* _iocp;
	int _loopMs = 10;

	int GetWorkTime() const { return oldWorkTime; }
private:

	bool EnqJob();
	bool DeqJob();

	bool NeedUpdate();
	void Update();
	void HandleEnter();
	void HandleLeave();
	void Execute(IOCP* iocp) const;
	void HandlePacket();

	template <typename Header>
	void RecvHandler(Session& session, void* iocp);


	void onRecvPacket(const Session& session, CRecvBuffer& buffer);
	
	long _leaveCount = 0;
	long _handledLeave = 0;
private:
	GroupID _groupId = GroupID::InvalidGroupID();
	GroupExecutable& _executable;
	std::chrono::steady_clock::time_point _nextUpdate;
	SessionSet _sessions;

	TlsLockFreeQueue<GroupJob> _jobs;

	TlsLockFreeQueue<SessionID> _enterSessions;
	TlsLockFreeQueue<SessionID> _leaveSessions;
	volatile char _isRun = 0;

	GroupManager* _owner;

	tuple<SessionID,String,thread::id> _debugLeave[1000];
	int debugIndex;


	//MONITOR
	std::chrono::steady_clock::time_point _nextMonitor;
	int workTime = 0;
	int oldWorkTime = 0;
};

