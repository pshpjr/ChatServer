#pragma once
#include "Executable.h"


class SendExecutable final : public Executable
{
public:
    SendExecutable():Executable{IoType::Send}{};

    void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
    ~SendExecutable() override = default;
};
