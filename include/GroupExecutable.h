#ifndef GROUPEXECUTABLE_H
#define GROUPEXECUTABLE_H
#include "Executable.h"


class Group;

class GroupExecutable : public Executable
{
public:
    GroupExecutable(Group* owner):Executable{ioType::GROUP},_owner{owner}
    {
    }

    void Execute(ULONG_PTR arg, DWORD transferred, void* iocp) override;

private:
    Group* _owner;
};
#endif // GROUPEXECUTABLE_H
