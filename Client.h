#pragma once
#include "Types.h"
#include "Group.h"

class Client: private Group
{
	friend class IOCP;
	friend class GroupManager;
public:
	virtual void onRecv(CRecvBuffer& recvBuffer) {};

	void Send(CSendBuffer& buffer);

private:


	void OnRecv(SessionID id, CRecvBuffer& recvBuffer) final;

	SessionID _id;
};


