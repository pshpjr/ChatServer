#pragma once

#include <chrono>
#include <queue>
#include "Types.h"
#include "Container.h"
#include "TLSLockFreeQueue.h"
#include "BuildOption.h"
#include "Session.h"
class CSendBuffer;
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
	friend class GroupExecutable;
	friend class GroupManager;
	friend class Client;
public:

	virtual void OnUpdate() {};
	virtual void OnEnter(SessionID id) {};
	virtual void OnLeave(SessionID id) {};
	virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) {};

protected:
	void SendPacket(SessionID id, CSendBuffer& buffer);
	void MoveSession(SessionID id, GroupID dst);
	void LeaveSession(SessionID id);
	void EnterSession(SessionID id);
	Group();
private:


	void Update();
	void handleEnter();
	void handleLeave();
	void execute(IOCP* iocp);
	void handlePacket();
private:

	GroupID _groupId = -1;
	GroupExecutable& _executable;
	std::chrono::system_clock::time_point _lastUpdate;
	HashSet<SessionID, SessionInfo::SessionIDHash, SessionInfo::SessionIDEqual> _sessions;
	TLSLockFreeQueue<SessionID> _enterSessions;
	TLSLockFreeQueue<SessionID> _leaveSessions;
	
	IOCP* _iocp;
	GroupManager* _owner;
};

