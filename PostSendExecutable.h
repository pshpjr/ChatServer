#pragma once
#include "Executable.h"
#include <chrono>

class PostSendExecutable final : public Executable
{
public:
	PostSendExecutable()
	{
		_type = POSTRECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~PostSendExecutable() override = default;

	std::chrono::system_clock::time_point lastSend;

	long dataNotSend = 0;
};

