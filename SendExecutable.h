#pragma once
#include "Executable.h"


class SendExecutable : public Executable
{
public:
	SendExecutable()
	{
		_type = ioType::SEND;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~SendExecutable() override = default;

};

