#pragma once
#include "Executable.h"
class Session;
class RecvExecutable : public Executable
{
public:
	RecvExecutable()
	{
		_type = ioType::RECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~RecvExecutable() override = default;

private:

	template <typename Header>
	void recvHandler(Session& session, void* iocp); 

};

