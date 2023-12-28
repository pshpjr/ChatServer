#pragma once
#include "Executable.h"
class Session;
class RecvExecutable final : public Executable
{
public:
	RecvExecutable()
	{
		_type = RECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~RecvExecutable() override = default;

private:

	template <typename Header>
	void RecvHandler(Session& session, void* iocp); 

};

