#include "Executable.h"
#pragma once

class Group;

class GroupExecutable : public Executable
{
public:
    GroupExecutable(Group* owner)
        : _owner(owner)
    {
    }

    void Execute(ULONG_PTR arg, DWORD transferred, void* iocp) override;

private:
    Group* _owner;
};
