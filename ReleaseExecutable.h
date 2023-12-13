#pragma once
#include "Executable.h"

class ReleaseExecutable : public Executable
{
public:
	ReleaseExecutable()
	{
		_type = ioType::RELEASE;
	}
	void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
	~ReleaseExecutable() override = default;

};


