#pragma once
#include "Executable.h"
class Session;

class RecvExecutable final : public Executable {
public:
    RecvExecutable() : Executable{IoType::Recv}{}

    void Execute(ULONG_PTR key, DWORD transferred, void* iocp) override;
    ~RecvExecutable() override = default;
    int _recvBufSize[2] = {0,};
    bool isPending = false;

private:
    template <typename Header>
    void RecvHandler(Session& session, void* iocp);
};
