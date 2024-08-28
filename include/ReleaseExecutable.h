#pragma once
#include "Executable.h"

class ReleaseExecutable final : public Executable
{
public:
    ReleaseExecutable()
    {
        _type = RELEASE;
    }

    void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
    ~ReleaseExecutable() override = default;
};
