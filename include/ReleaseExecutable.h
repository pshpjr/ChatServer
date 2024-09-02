#pragma once
#include "Executable.h"

class ReleaseExecutable final : public Executable
{
public:
    ReleaseExecutable():Executable{IoType::Release}{};

    void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
    ~ReleaseExecutable() override = default;
};
