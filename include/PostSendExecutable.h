#pragma once
#include <chrono>
#include "Executable.h"

class PostSendExecutable final : public Executable
{
public:
    PostSendExecutable():Executable{IoType::Postrecv}{}

    void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
    ~PostSendExecutable() override = default;

    std::chrono::steady_clock::time_point lastSend{};

    long dataNotSend = 0;
};
