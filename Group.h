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
	virtual void OnEnter(SessionID id, void* optionVal) {};
	virtual void OnLeave(SessionID id) {};
	virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) {};
	virtual ~Group() {};
protected:
	void SendPacket(SessionID id, SendBuffer& buffer) const;
	void MoveSession(SessionID id, GroupID dst) const;
	void LeaveSession(SessionID id);
	void EnterSession(SessionID id);
	void SetLoopMs(int loopMS);
	Group();
	IOCP* _iocp;
private:

	bool NeedUpdate();
	void Update();
	void HandleEnter();
	void HandleLeave();
	void Execute(IOCP* iocp) const;
	void HandlePacket();

	template <typename Header>
	void RecvHandler(Session& session, void* iocp);


	void onRecvPacket(const Session& session, CRecvBuffer& buffer);
	

private:
	GroupID _groupId = -1;
	GroupExecutable& _executable;
	std::chrono::steady_clock::time_point _nextUpdate;
	SessionSet _sessions;
	TlsLockFreeQueue<SessionID> _enterSessions;
	TlsLockFreeQueue<SessionID> _leaveSessions;
	char _isRun = 0;
	int _loopMs = 10;
	GroupManager* _owner;
};

