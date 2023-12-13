#pragma once
#include "Executable.h"
#include <chrono>

class PostSendExecutable : public Executable
{
public:
	PostSendExecutable()
	{
		_type = ioType::POSTRECV;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~PostSendExecutable() override = default;

	std::chrono::system_clock::time_point _lastSend;

	long dataNotSend = 0;
};

